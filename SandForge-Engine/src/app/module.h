#pragma once
#include <string>


class App;

class Module
{
public:
	virtual ~Module() = default;

	App* app;

	Module(App* parent, bool start_enabled = true) : app(parent), active(start_enabled)
	{
	}

	void Enable()
	{
		if (active == false)
		{
			active = true;
			Start();
		}
	}

	void Disable()
	{
		if (active == true)
		{
			active = false;
			CleanUp();
		}
	}

	void Init()
	{
		active = true;
	}

	// Called before render is available
	virtual bool Awake()
	{
		return true;
	}

	// Called before the first frame
	virtual bool Start()
	{
		return true;
	}

	// Called each loop before update
	virtual bool PreUpdate()
	{
		return true;
	}

	// Called each loop iteration
	virtual bool Update(float dt)
	{
		return true;
	}

	// Called each loop after update
	virtual bool PostUpdate()
	{
		return true;
	}

	// Called before quitting
	virtual bool CleanUp()
	{
		return true;
	}

	virtual int GetSceneNumber() {
		return -10;
	}

public:

	std::string name;
	bool sceneReload = false;
	bool active;

};