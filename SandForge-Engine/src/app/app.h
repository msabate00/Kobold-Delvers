#pragma once
#include "app/defs.h"
#include "app/game_progress.h"
#include <string>

class GLFWwindow;

class Engine;
class Renderer;
class Input;
class UI;
class Audio;
class Settings;
class SceneManager;
class Particles;


struct Camera2D { 
	Vec2<float> pos{ 0,0 }; 
	Vec2<float> size{ 320,180 }; 

};


class App
{
public:

	// Constructor
	App(int argc, char* args[]);

	// Destructor
	virtual ~App();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called each loop iteration
	bool Update();

	// Called before quitting
	bool CleanUp();

	void SetCameraRect(float x, float y, float w, float h);

	void RequestQuit(); //Close app
	void RequestScreenshot(bool transparent);
	bool HasPendingOpaqueScreenshot() const { return !pendingOpaqueScreenshotPath.empty(); }
	bool HasPendingTransparentScreenshot() const { return !pendingTransparentScreenshotPath.empty(); }
	const std::string& PendingOpaqueScreenshotPath() const { return pendingOpaqueScreenshotPath; }
	const std::string& PendingTransparentScreenshotPath() const { return pendingTransparentScreenshotPath; }
	void FinishOpaqueScreenshot(bool success);
	void FinishTransparentScreenshot(bool success);

	GLFWwindow* GetWindow() const { return window; }
	void ResetCamera();

public:
	Engine* engine;
	Renderer* renderer;
	Input* input;
	UI* ui;
	Audio* audio;
	Settings* settings;
	SceneManager* scenes;
	Particles* particles;

	GameProgress progress;
	float screenFade = 0.0f;

	int pixelsPerCell = 4;
	Vec2<int> framebufferSize{ 1280, 720 };
	Vec2<int> windowSize{ 1280, 720 };
	Vec2<int> gridSize{ 920, 300 };

	int frames; //para los randBit determinista y contador de fps
	float dt;
	uint8 showChunks = 0;
	uint8 showHitbox = 0;
	uint8 showGridBounds = 0;
	bool shiftPressed = false;

	Camera2D camera;

private:
	int argc;
	char** args;

	

	GLFWwindow* window;
	bool quitRequested = false;
	std::string pendingOpaqueScreenshotPath;
	std::string pendingTransparentScreenshotPath;

};

extern App* app;