#pragma once
#include "app/defs.h"

class GLFWwindow;

class Engine;
class Renderer;
class Input;
class UI;
class Audio;
class SceneManager;


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

public:
	Engine* engine;
	Renderer* renderer;
	Input* input;
	UI* ui;
	Audio* audio;
	SceneManager* scenes;

	int pixelsPerCell = 4;
	Vec2<int> framebufferSize{ 1280, 720 };
	Vec2<int> windowSize{ 1280, 720 };
	Vec2<int> gridSize{ 920, 300 };

	int frames; //para los randBit determinista y contador de fps
	float dt;
	uint8 showChunks = 0;

	Camera2D camera;

private:
	int argc;
	char** args;

	

	GLFWwindow* window;
	bool quitRequested = false;

};

extern App* app;