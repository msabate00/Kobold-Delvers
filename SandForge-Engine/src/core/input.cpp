#include "input.h"
#include "../core/engine.h"
#include "../core/material.h"
#include "../app/app.h"
#include "../ui/ui.h"
#include <algorithm>

Input::Input(App* app, bool start_enabled) : Module(app, start_enabled) {};
Input::~Input() = default;

bool Input::Awake() { return true; }
bool Input::Start() { return true; }
bool Input::PreUpdate() { return true; }
bool Input::Update(float) { return true; }
bool Input::PostUpdate() { return true; }
bool Input::CleanUp() { return true; }

void Input::SetupWindow(GLFWwindow* window)
{
    this->window = window;
    glfwSetWindowUserPointer(this->window, this);
    glfwSetKeyCallback(this->window, &Input::SKey);
    glfwSetMouseButtonCallback(this->window, &Input::SMouseBtn);
    glfwSetScrollCallback(this->window, &Input::SScroll);

    glfwSetWindowSizeCallback(this->window, &Input::SWindowSize);
    glfwSetFramebufferSizeCallback(this->window, &Input::SFramebufferSize);

    
}

void Input::BeginFrame()
{
    glfwGetCursorPos(window, &mx, &my);
}

void Input::EndFrame() {


    scrollYSteps = (int)scrollYAccum;
    scrollYAccum -= (double)scrollYSteps;

    std::fill(std::begin(pressed), std::end(pressed), false);
    std::fill(std::begin(released), std::end(released), false);
    std::fill(std::begin(mouseUp), std::end(mouseUp), false);

}

void Input::ProcessBindings(Material& brushMat, int& brushSize) {
    if (this->KeyDown(GLFW_KEY_1)) brushMat = Material::Sand;
    if (this->KeyDown(GLFW_KEY_2)) brushMat = Material::Water;
    if (this->KeyDown(GLFW_KEY_3)) brushMat = Material::Stone;
    if (this->KeyDown(GLFW_KEY_4)) brushMat = Material::Wood;
    if (this->KeyDown(GLFW_KEY_5)) brushMat = Material::Fire;
    if (this->KeyDown(GLFW_KEY_6)) brushMat = Material::Smoke;
    if (this->KeyDown(GLFW_KEY_9)) brushMat = Material::Empty;

    if (this->KeyDown(GLFW_KEY_P)) app->engine->paused = !app->engine->paused;
    if (this->KeyDown(GLFW_KEY_N)) app->engine->stepOnce = true;

    if (this->KeyDown(GLFW_KEY_F1)) app->showChunks = ~app->showChunks;
    if (this->KeyDown(GLFW_KEY_F2)) app->showHitbox = ~app->showHitbox;

    if (this->KeyDown(GLFW_KEY_F5)) app->engine->SaveLevel("levels/custom/quick.lvl");
    if (this->KeyDown(GLFW_KEY_F9)) app->engine->LoadLevel("levels/custom/quick.lvl");
    if (this->KeyDown(GLFW_KEY_F6)) app->engine->ClearWorld((uint8)Material::Empty);


    float sp = 300 * app->dt;
    if (KeyRepeat(GLFW_KEY_A)) app->SetCameraRect(app->camera.pos.x - sp, app->camera.pos.y , app->camera.size.x, app->camera.size.y);
    if (KeyRepeat(GLFW_KEY_D)) app->SetCameraRect(app->camera.pos.x + sp, app->camera.pos.y, app->camera.size.x, app->camera.size.y);
    if (KeyRepeat(GLFW_KEY_W)) app->SetCameraRect(app->camera.pos.x, app->camera.pos.y - sp, app->camera.size.x, app->camera.size.y);
    if (KeyRepeat(GLFW_KEY_S)) app->SetCameraRect(app->camera.pos.x, app->camera.pos.y + sp, app->camera.size.x, app->camera.size.y);



	const bool shift = KeyRepeat(GLFW_KEY_LEFT_SHIFT) || KeyRepeat(GLFW_KEY_RIGHT_SHIFT);

	if (this->MouseDown(GLFW_MOUSE_BUTTON_1)) {
		if (!app->ui->ConsumedMouse()) {
			app->engine->Paint((int)MouseX(), (int)MouseY(), brushMat, brushSize, shift);
		}
	}
    if (this->MouseUp(GLFW_MOUSE_BUTTON_1)) {
        app->engine->StopPaint();
    }

    if (this->ScrollSteps() != 0) {
        const int steps = this->ScrollSteps();

        //Para zoom de la camara
        const bool ctrl = KeyRepeat(GLFW_KEY_LEFT_CONTROL);
        if (ctrl) {
            constexpr int kMinPpc = 1;
            constexpr int kMaxPpc = 32;

            //Mantener en el centro
            const float cx = app->camera.pos.x + app->camera.size.x * 0.5f;
            const float cy = app->camera.pos.y + app->camera.size.y * 0.5f;

            //Aumentamos los pixeles
            app->pixelsPerCell = std::clamp(app->pixelsPerCell + steps, kMinPpc, kMaxPpc);

            //Ajustamos la resolu
            const float camW = app->framebufferSize.x / (float)app->pixelsPerCell;
            const float camH = app->framebufferSize.y / (float)app->pixelsPerCell;
            app->SetCameraRect(cx - camW * 0.5f, cy - camH * 0.5f, camW, camH);
        }
        else {
            
            brushSize += steps;
            brushSize = std::fmax(1, std::fmin(64, brushSize));
        }
    }
}

double Input::MouseX() const {
    int ww = std::fmax(1, app->windowSize.x);
    int fw = std::fmax(1, app->framebufferSize.x);
    return mx * (double)fw / (double)ww;
}

double Input::MouseY() const {
    int wh = std::fmax(1, app->windowSize.y);
    int fh = std::fmax(1, app->framebufferSize.y);
    return my * (double)fh / (double)wh;
}

void Input::SWindowSize(GLFWwindow* w, int ww, int wh) {
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(w));
    if (!self || !self->app) return;
    self->app->windowSize = { ww, wh };
}

void Input::SFramebufferSize(GLFWwindow* w, int fw, int fh) {
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(w));
    if (!self || !self->app) return;

    self->app->framebufferSize = { fw, fh };

    float camW = fw / (float)self->app->pixelsPerCell;
    float camH = fh / (float)self->app->pixelsPerCell;
    self->app->SetCameraRect(self->app->camera.pos.x, self->app->camera.pos.y, camW, camH);
}

void Input::SKey(GLFWwindow* w, int key, int, int action, int) {
    if (key < 0 || key >= 512) return;
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(w));
    if (!self) return;

    switch (action) {
    case GLFW_PRESS:
        self->keys[key] = true;
        self->pressed[key] = true;
        break;
    case GLFW_REPEAT:
        self->keys[key] = true;
        break;
    case GLFW_RELEASE:
        self->keys[key] = false;
        self->released[key] = true;
        break;
    }
}

void Input::SMouseBtn(GLFWwindow* w, int button, int action, int) {
    if (button < 0 || button >= 8) return;
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(w));
    if (!self) return;
    if (action == GLFW_PRESS) self->mouseDown[button] = true;
    else if (action == GLFW_RELEASE) {
        self->mouseDown[button] = false;
        self->mouseUp[button] = true;
    } 
}

void Input::SScroll(GLFWwindow* w, double, double yoff) {
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(w));
    if (!self) return;
    self->scrollYAccum += yoff;
}
