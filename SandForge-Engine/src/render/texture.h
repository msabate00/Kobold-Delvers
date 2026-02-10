#pragma once
#include <app/defs.h>


struct Texture2D {
	uint id = 0; int w = 0, h = 0;
	bool Load(const char* path);
	bool Create(int w, int h, const void* rgbaBytes, bool linearFilter = false);
	void Destroy();
};