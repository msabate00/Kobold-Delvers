#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "app/app.h"
#include <algorithm>
#include <chrono>
#include "app/timer.h"
#include "core/engine.h"
#include "core/input.h"
#include "render/renderer.h"
#include "ui/ui.h"
#include "game/scene_manager.h"
#include "app/settings.h"
#include "app/log.h"



// Constructor
App::App(int argc, char* args[]) : argc(argc), args(args)
{
	engine = new Engine(this);
	renderer = new Renderer(this);
	input = new Input(this);
	ui = new UI(this);
	audio = new Audio(this);
	settings = new Settings(this);
	scenes = new SceneManager(this);
}


App::~App()
{
	if (engine || renderer || input || ui || audio || settings || scenes || window) {
		CleanUp();
	}
}


bool App::Awake()
{
	bool ret = true;

	if (!glfwInit()) {
		LOG("ERROR: glfwInit() failed");
		return false;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	if (settings) {
		settings->Load();
		windowSize.x = std::fmax(64, settings->data.windowW);
		windowSize.y = std::fmax(64, settings->data.windowH);
	}

	window = glfwCreateWindow(windowSize.x, windowSize.y, "Kobold Delvers", nullptr, nullptr);
	if (!window) {
		LOG("ERROR: glfwCreateWindow(%d,%d) failed", windowSize.x, windowSize.y);
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);

	glfwGetWindowSize(window, &windowSize.x, &windowSize.y);
	glfwGetFramebufferSize(window, &framebufferSize.x, &framebufferSize.y);

	if (!gladLoadGL(glfwGetProcAddress)) {
		LOG("ERROR: gladLoadGL() failed");
		return false;
	}

	LOG("Awake: Settings");
	if (settings) settings->Awake();

	LOG("OpenGL initialized. Vendor=%s | Renderer=%s | Version=%s",
		(const char*)glGetString(GL_VENDOR),
		(const char*)glGetString(GL_RENDERER),
		(const char*)glGetString(GL_VERSION));

	LOG("Awake: Engine");
	engine->Awake();
	registerDefaultMaterials();
	LOG("Awake: Renderer");
	renderer->Awake();
	LOG("Awake: UI");
	ui->Awake();

	LOG("Awake: Input");
	input->Awake();
	input->SetupWindow(window);
	Input::SFramebufferSize(window,framebufferSize.x, framebufferSize.y);

	LOG("Awake: Audio");
	audio->Awake();
	if (settings) settings->ApplyAudio();
	LOG("Awake: SceneManager");
	scenes->Awake();

	


	return ret;
}

bool App::Start()
{
	bool ret = true;
	LOG("Start: Settings");
	if (settings) settings->Start();
	LOG("Start: Engine");
	engine->Start();
	LOG("Start: Renderer");
	renderer->Start();
	LOG("Start: Input");
	input->Start();
	LOG("Start: UI");
	ui->Start();
	LOG("Start: Audio");
	audio->Start(); //se puede quitar
	LOG("Start: SceneManager");
	scenes->Start();

	camera.pos.y = gridSize.y - camera.size.y;

	return ret;
}

bool App::Update()
{
	Timer timer = Timer();
	double fpsTimer = 0.0;
	frames = 0;

	Material brushMat = Material::Sand;
	int brushSize = 2;

	while (!glfwWindowShouldClose(window) && !quitRequested)
	{
		glfwPollEvents();

		dt = timer.Read();
		timer.Start();

		input->BeginFrame();
		scenes->BeginFrame();

		// Mouse hide/show solo en escenas de mundo
		if (scenes->WorldActive()) {
			if (input->MouseY() > 50) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			} 
			else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}                  
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		ui->SetMouse(input->MouseX(), input->MouseY(), input->MouseDown(GLFW_MOUSE_BUTTON_1));

		// UI logica
		ui->SetNoRender(true);
		ui->Begin(app->framebufferSize.x, app->framebufferSize.y);
		scenes->DrawUI(brushSize, brushMat);
		ui->End();

		scenes->BeginFrame();
		scenes->Update(dt);
		scenes->BeginFrame();

		if (scenes->WorldActive()) {
			input->ProcessBindings(brushMat, brushSize);
			engine->Update(dt);
		}

		audio->Update(dt); //se puede quitar
		renderer->Update(dt);

		// UI render
		ui->SetNoRender(false);
		ui->Begin(app->framebufferSize.x, app->framebufferSize.y);
		scenes->DrawUI(brushSize, brushMat);
		ui->End();
		renderer->FlushUI(app->framebufferSize.x, app->framebufferSize.y);

		input->EndFrame();

		frames++;
		fpsTimer += dt;
		if (fpsTimer >= 1.0) {
			double fps = frames / fpsTimer;
			char buf[128];
			std::snprintf(buf, sizeof(buf), "Kobold Delvers - %.1f FPS", fps);
			glfwSetWindowTitle(window, buf);
			fpsTimer = 0.0;
			frames = 0;
		}
		glfwSwapBuffers(window);
	}


	return false;
}

bool App::CleanUp()
{
	bool ret = true;

	if (scenes)  { scenes->CleanUp();  delete scenes;  scenes = nullptr; }
	if (settings){ settings->CleanUp();delete settings;settings = nullptr; }
	if (ui)      { ui->CleanUp();      delete ui;      ui = nullptr; }
	if (input)   { input->CleanUp();   delete input;   input = nullptr; }
	if (renderer){ renderer->CleanUp();delete renderer;renderer = nullptr; }
	if (engine)  { engine->CleanUp();  delete engine;  engine = nullptr; }
	if (audio)   { audio->CleanUp();   delete audio;   audio = nullptr; }

	if (window)  { glfwDestroyWindow(window); window = nullptr; }
	glfwTerminate();

	return ret;
}


void App::RequestQuit()
{
	quitRequested = true;
	if (window) glfwSetWindowShouldClose(window, 1);
}


void App::SetCameraRect(float x, float y, float w, float h) {
	camera.size.x = std::fmax(1.f, w);
	camera.size.y = std::fmax(1.f, h);
	camera.size.x = std::fmin(camera.size.x, (float)gridSize.x);
	camera.size.y = std::fmin(camera.size.y, (float)gridSize.y);

	float maxX = (float)gridSize.x - camera.size.x;
	float maxY = (float)gridSize.y - camera.size.y;

	camera.pos.x = std::clamp(x, 0.f, maxX);
	camera.pos.y = std::clamp(y, 0.f, maxY);
}
