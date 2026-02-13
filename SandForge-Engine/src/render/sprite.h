#pragma once
#include <vector>
#include <cstdint>
#include "app/defs.h"

struct Texture2D;

enum class RenderLayer { BG, WORLD, UI };
struct Sprite {
	const Texture2D* tex = nullptr;
	float x = 0, y = 0, w = 0, h = 0;      
	float u0 = 0, v0 = 0, u1 = 1, v1 = 1; 
	uint32 color = 0xFFFFFFFF;     
	RenderLayer layer = RenderLayer::WORLD;
	int sort = 0;
};

class SpriteBatch {
	uint prog = 0, vao = 0, vbo = 0; int loc_uView = -1;
	struct V { float x, y, u, v; uint32 rgba; };
	int viewW = 0, viewH = 0;
	std::vector<V> verts;
	std::vector<Sprite> queue;
	std::vector<const Sprite*> tmp;
public:
	void Init();
	void Shutdown();
	void Begin(int viewW, int viewH);
	void Push(const Sprite& s);
	void Flush(RenderLayer layer);
};