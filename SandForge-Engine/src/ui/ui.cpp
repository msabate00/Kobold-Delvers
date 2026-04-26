#include "ui/ui.h" 
#include <glad/gl.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "app/module.h"  
#include "app/app.h"
#include "audio/audio.h"
#include "core/utils.h"
#include "core/engine.h"
#include "render/renderer.h"
#include "app/defs.h"


static inline void PushTri(std::vector<Vertex>& v,
    float x0, float y0, float x1, float y1, float x2, float y2, uint32 c)
{
    v.push_back({ x0, y0, c });
    v.push_back({ x1, y1, c });
    v.push_back({ x2, y2, c });
}


UI::UI(App* app, bool start_enabled) : Module(app, start_enabled) {};
UI::~UI() = default;


bool UI::Awake() {

	std::string vsSrc = ReadTextFile(SHADER_DIR "/ui.vs.glsl");
	std::string fsSrc = ReadTextFile(SHADER_DIR "/ui.fs.glsl");

	prog = MakeProgram(vsSrc.c_str(), fsSrc.c_str());
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 4 * 1024 * 1024, nullptr, GL_DYNAMIC_DRAW);


	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);


	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, rgba));


	glBindVertexArray(0);
	loc_uView = glGetUniformLocation(prog, "uView");
    return true;

}
bool UI::Start() {
	fontReady = font.Load("assets/fonts/PixeloidSans.ttf", 24.0f, 512, 512, false); //cambiar pixel height para mayor calidad
	matAtlas.Load(SPRITE_DIR "/materialAtlas.png");
	cursorTex.Load(SPRITE_DIR "/cursor.png");
	interfaceTex.Load(SPRITE_DIR "/UI/interface.png");
	npcInteractionsTex.Load(SPRITE_DIR "/KoboldInteractions.png");
	tutorialsTex.Load(SPRITE_DIR "/Tutorials.png");
	return true;
}
bool UI::PreUpdate() { return true; }

bool UI::Update(float dt) {
    return true;
}
bool UI::PostUpdate() { return true; }
bool UI::CleanUp() { 
	font.Destroy();
	fontReady = false;
	matAtlas.Destroy();
	interfaceTex.Destroy();
	npcInteractionsTex.Destroy();
	tutorialsTex.Destroy();
	cursorTex.Destroy();

	if (vbo) glDeleteBuffers(1, &vbo);
	if (vao) glDeleteVertexArrays(1, &vao);
	if (prog) glDeleteProgram(prog);
	vbo = vao = prog = 0;

	return true;

}

void UI::Begin(int viewW, int viewH) {
	vw = viewW; vh = viewH; verts.clear();
	if (noRender) {
		hoverItemCounter = 0;
		hoveredItemCurrent = 0;
		hoverSoundCandidate = 0;
	}
}

void UI::Draw(int& brushSize, Material& brushMat) {

	Material auxMat = brushMat;
	if (app->engine->HasPlayer()) {
		if (const Player* player = app->engine->GetPlayer()) {
			if (player->heldMaterial >= Material::Sand && player->heldMaterial <= Material::Dynamite) {
				auxMat = player->heldMaterial;
			}
		}
	}
	const MatProps actualMat = matProps((uint8)auxMat);

	

	float ringX = (float)mx;
	float ringY = (float)my;
	int ringRadius = brushSize;
	if (app->engine->HasPlayer()) {
		ringRadius = 2;
		int wx = 0;
		int wy = 0;
		if (app->engine->GetPlayerPlaceTargetCellFromMouse((int)mx, (int)my, wx, wy)) {
			float sx = 0.0f, sy = 0.0f, sw = 0.0f, sh = 0.0f;
			if (app->engine->WorldRectToScreen((float)wx, (float)wy, 1.0f, 1.0f,
				app->framebufferSize.x, app->framebufferSize.y, sx, sy, sw, sh)) {
				ringX = std::floor(sx + sw * 0.5f);
				ringY = std::floor(sy + sh * 0.5f);
				LevelCellsProtectionMark(ringRadius, ringX, ringY);
			}
		}
	}

	Ring(ringX, ringY, ringRadius, 2, RGBAu32(actualMat.color.r, actualMat.color.g, actualMat.color.b, 100), 12);

	if (app->engine->levelCellsProtection && !app->engine->HasPlayer()) {
		LevelCellsProtectionMark(brushSize, mx, my);
	}

	Cursor();

	if (app->showChunks) {
		std::vector<uint> chunks = app->engine->GetChunks();
		for (int ci = 0; ci < (int)chunks.size(); ++ci) {
			int x, y, w, h;
			app->engine->GetChunkRect(ci, x, y, w, h);

			float sx, sy, sw, sh;
			if (!app->engine->WorldRectToScreen(
				(float)x, (float)y, (float)w, (float)h,
				app->framebufferSize.x, app->framebufferSize.y,
				sx, sy, sw, sh))
				continue;

			if (chunks[ci]) RectBorders(sx, sy, sw, sh, 4.0f, RGBAu32(230, 130, 130, 200));
			else            RectBorders(sx, sy, sw, sh, 2.0f, RGBAu32(230, 230, 230, 100));
		}
	}

	if (app->showHitbox) {
		std::vector<NPC> npcs = app->engine->GetNPCs();
		for (const NPC& n : npcs) {
			if (!n.alive) continue;

			// Caja NPC
			{
				float sx, sy, sw, sh;
				if (app->engine->WorldRectToScreen(
					(float)n.x, (float)n.y, (float)n.w, (float)n.h,
					app->framebufferSize.x, app->framebufferSize.y,
					sx, sy, sw, sh))
				{
					RectBorders(sx, sy, sw, sh, 2.0f, RGBAu32(80, 220, 80, 200));
				}
			}

			// Hitbox
			{
				int hx = n.x + n.hbOffX;
				int hy = n.y + n.hbOffY;

				float sx, sy, sw, sh;
				if (app->engine->WorldRectToScreen(
					(float)hx, (float)hy, (float)n.hbW, (float)n.hbH,
					app->framebufferSize.x, app->framebufferSize.y,
					sx, sy, sw, sh))
				{
					RectBorders(sx, sy, sw, sh, 2.0f, RGBAu32(230, 80, 80, 220));
				}
			}
		}

		const auto& spawners = app->engine->GetSpawners();
		for (const NPCSpawner& sp : spawners) {
			float sx, sy, sw, sh;
			if (app->engine->WorldRectToScreen(
				(float)sp.x, (float)sp.y, (float)sp.w, (float)sp.h,
				app->framebufferSize.x, app->framebufferSize.y,
				sx, sy, sw, sh))
			{
				RectBorders(sx, sy, sw, sh, 2.0f, RGBAu32(80, 220, 255, 220));
			}
		}

		const auto& goalsDbg = app->engine->GetGoals();
		for (const NPCGoal& g : goalsDbg) {
			float sx, sy, sw, sh;
			if (app->engine->WorldRectToScreen(
				(float)g.x, (float)g.y, (float)g.w, (float)g.h,
				app->framebufferSize.x, app->framebufferSize.y,
				sx, sy, sw, sh))
			{
				RectBorders(sx, sy, sw, sh, 2.0f, RGBAu32(255, 220, 80, 220));
			}
		}

		const auto& bonuses = app->engine->GetBonuses();
		for (const NPCBonus& b : bonuses) {
			float sx, sy, sw, sh;
			if (app->engine->WorldRectToScreen(
				(float)b.x, (float)b.y, (float)b.w, (float)b.h,
				app->framebufferSize.x, app->framebufferSize.y,
				sx, sy, sw, sh))
			{
				RectBorders(sx, sy, sw, sh, 2.0f, RGBAu32(160, 90, 230, 220));
			}
		}
	}

	const auto& goals = app->engine->GetGoals();
	for (const NPCGoal& g : goals) {
		if (g.capturedCount <= 0) continue;

		float sx, sy, sw, sh;
		if (!app->engine->WorldRectToScreen(
			(float)g.x, (float)g.y, (float)g.w, (float)g.h,
			app->framebufferSize.x, app->framebufferSize.y,
			sx, sy, sw, sh))
			continue;

		char buf[32];
		std::snprintf(buf, sizeof(buf), "%d", g.capturedCount);
		TextCentered(sx, sy - 14.0f, sw, 12.0f, buf, RGBAu32(255, 245, 180, 240), 0.65f);
	}

	
	if (const Player* player = app->engine->GetPlayer()) {
		if (player->alive) {
			float sx, sy, sw, sh;
			if (app->engine->WorldRectToScreen(
				(float)player->x, (float)player->y, (float)player->w, (float)player->h,
				app->framebufferSize.x, app->framebufferSize.y,
				sx, sy, sw, sh))
			{  
				const float boxW = 42.0f; 
				const float boxH = 26.0f;
				const float boxX = std::floor(sx + (sw - boxW) * 0.5f);
				const float boxY = std::floor(sy - boxH - 8.0f);

				if (player->drowning && player->oxygenTime > 0.0f) {
					Image(npcInteractionsTex, boxX + 30, boxY + 23, boxW + 20, boxH, speechBoxRed, RGBAu32(255, 255, 255, 255), 3);
					Text(boxX + 36.0f, boxY + 26.0f, "GLUB!", RGBAu32(255, 245, 245, 255), 0.45f);

					const float progress = Clamp01(player->oxygenTime / 3);
					const int stage = std::fmin(15, (int)std::floor(progress * 16.0f));

					for (int b = 0; b < 3; ++b) {
						int bubbleStage = stage - b * 5;
						bubbleStage = std::clamp(bubbleStage, 0, 5);
						Image(npcInteractionsTex, boxX + b * 10.0f + 60, boxY + 26, 10.0f, 10.0f, oxygenBubble[(size_t)bubbleStage], RGBAu32(255, 255, 255, 255), 4);
					}
				}
			}
		}
	}

	const auto& npcs = app->engine->GetNPCs();
	for (const NPC& n : npcs) {
		if (!n.alive) continue;

		float sx, sy, sw, sh;
		if (!app->engine->WorldRectToScreen(
			(float)n.x, (float)n.y, (float)n.w, (float)n.h,
			app->framebufferSize.x, app->framebufferSize.y,
			sx, sy, sw, sh))
			continue;

		const float boxW = 42.0f;
		const float boxH = 26.0f;
		const float boxX = std::floor(sx + (sw - boxW) * 0.5f);
		const float boxY = std::floor(sy - boxH - 8.0f);

		if (n.drowning && n.oxygenTime > 0.0f) {
			Image(npcInteractionsTex, boxX+30, boxY+23, boxW+20, boxH, speechBoxRed, RGBAu32(255, 255, 255, 255), 3);
			Text(boxX + 6.0f + 30, boxY + 26.0f, "GLUB!", RGBAu32(255, 245, 245, 255), 0.45f);

			const float progress = Clamp01(n.oxygenTime / 3);
			const int stage = std::fmin(15, (int)std::floor(progress * 48.0f));

			for (int b = 0; b < 3; ++b) {
				int bubbleStage = stage - b * 5;
				bubbleStage = std::clamp(bubbleStage, 0, 5);
				Image(npcInteractionsTex, boxX + b * 10.0f + 30 + 30, boxY + 26, 10.0f, 10.0f, oxygenBubble[(size_t)bubbleStage], RGBAu32(255, 255, 255, 255), 4);
			}
			continue;
		}

		if (n.speechTimer > 0.0f && n.speechMessage >= 0 && n.speechMessage < (int)npcsMessages.size()) {
			Image(npcInteractionsTex, boxX+30, boxY + 23, boxW, boxH, speechBoxWhite, RGBAu32(255, 255, 255, 255), 3);
			TextCentered(boxX + 6.0f + 30, boxY + 23.0f, boxW - 12.0f, boxH - 8.0f,
				npcsMessages[(size_t)n.speechMessage], RGBAu32(35, 30, 25, 255), 0.42f);
		}
	}
	
	if (app->showGridBounds) {
		RectBordersWorld(0, 0, app->gridSize.x, app->gridSize.y, 2.0f, RGBAu32(120, 220, 255, 200));
	}

	if (app->shiftPressed && !md) {
		Rect(mx-(app->framebufferSize.x * 1.5f), my, app->framebufferSize.x*3, 1, RGBAu32(220, 220, 220, 100));
		Rect(mx, my-(app->framebufferSize.y * 1.5f), 1, app->framebufferSize.y * 3, RGBAu32(220, 220, 220, 100));
	}
}

void UI::End() {
	if (noRender) {
		if (hoverSoundCandidate != 0 && hoverSoundCandidate == hoveredItemCurrent && app && app->audio) {
			app->audio->PlayOneShot("hover_button", 1.0f);
		}
		hoveredItemPrev = hoveredItemCurrent;
		return;
	}
	Flush();
}

void UI::SetMouse(double x, double y, bool down) {
	mdPrev = md; md = down; mx = x; my = y; mouseConsumed = false;
}

bool UI::HasVisibleAlpha(uint32 rgba)
{
	return ((rgba >> 24) & 255u) != 0u;
}

void UI::RegisterHoverItem(bool hover, bool audible)
{
	if (!noRender) return;
	const uint32 id = ++hoverItemCounter;
	if (!hover) return;

	hoveredItemCurrent = id;
	if (audible && hoveredItemPrev != id) {
		hoverSoundCandidate = id;
	}
}

void UI::Flush() {
	if (verts.empty()) return;
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // por si acaso
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(prog);
	glUniform2f(loc_uView, (float)vw, (float)vh);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(Vertex), verts.data());
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLES, 0, (GLint)verts.size());
}

bool UI::SliderLogic(float x, float y, float w, float h,
	float minv, float maxv, float& v,
	float& t, float& kx, float& kx0, float& kw)
{
	if (v < minv) v = minv;
	if (v > maxv) v = maxv;

	float range = maxv - minv;
	t = (range > 0.0f) ? Clamp01((v - minv) / range) : 0.0f;

	kx = x + t * w;
	kw = h;
	kx0 = kx - kw * 0.5f;

	bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);

	if (hover && md) {
		float nt = (w > 0.0f) ? Clamp01(float((mx - x) / w)) : 0.0f;
		v = minv + nt * (maxv - minv);
		mouseConsumed = true;
	}

	return hover && !md && mdPrev;
}


void UI::LevelCellsProtectionMark(int brushSize, int mx, int my)
{
	int cx = 0, cy = 0;
	if (app->engine->ScreenToWorldCell((int)mx, (int)my, cx, cy)) {
		const int rr = std::fmax(1, brushSize);
		const int r2 = rr * rr;
		const int xmin = std::fmax(0, cx - rr);
		const int xmax = std::fmin(app->engine->GridW() - 1, cx + rr);
		const int ymin = std::fmax(0, cy - rr);
		const int ymax = std::fmin(app->engine->GridH() - 1, cy + rr);

		for (int wy = ymin; wy <= ymax; ++wy) {
			for (int wx = xmin; wx <= xmax; ++wx) {
				const int dx = wx - cx;
				const int dy = wy - cy;
				if (dx * dx + dy * dy > r2) continue;

				const Cell c = app->engine->GetCell(wx, wy);
				if (c.fromLevel == 1) {
					float sx = 0.0f, sy = 0.0f, sw = 0.0f, sh = 0.0f;
					if (app->engine->WorldRectToScreen((float)wx, (float)wy, 1.0f, 1.0f,
						app->framebufferSize.x, app->framebufferSize.y,
						sx, sy, sw, sh)) {

						Rect(sx, sy, sw, sh, RGBAu32(255, 120, 120, 55));
						RectBorders(sx, sy, sw, sh, 2, RGBAu32(255, 120, 120, 70));
					}
				}
			}
		}
	}
}



void UI::Rect(float x, float y, float w, float h, uint32 c) {
	if (noRender) return;
	Vertex v[6] = {
	{x, y, c}, {x + w, y, c}, {x + w, y + h, c},
	{x, y, c}, {x + w, y + h, c}, {x, y + h, c},
	};
	verts.insert(verts.end(), v, v + 6);
}

void UI::RectBorders(float x, float y, float w, float h, float t, uint32 rgba)
{
	if (noRender) return;
	float tM = (t / 2);
	Rect(x - tM, y - tM, w, tM, rgba);
	Rect(x - tM, (y + h) - tM, w, tM, rgba);

	Rect(x - tM, y - tM, tM, h, rgba);
	Rect((x + w) - tM, y - tM, tM, h, rgba);

}
void UI::RectBordersWorld(int x, int y, int w, int h, float thickness, uint32 color){
	int rx = std::fmax(x, int(app->camera.pos.x));
	int ry = std::fmax(y, int(app->camera.pos.y));
	int rw = std::fmin(x + w, int(app->camera.pos.x + app->camera.size.x)) - rx;
	int rh = std::fmin(y + h, int(app->camera.pos.y + app->camera.size.y)) - ry;
	if (rw <= 0 || rh <= 0) return;

	float sx = ((rx - app->camera.pos.x) / app->camera.size.x) * app->framebufferSize.x;
	float sy = ((ry - app->camera.pos.y) / app->camera.size.y) * app->framebufferSize.y;
	float sw = (rw / app->camera.size.x) * app->framebufferSize.x;
	float sh = (rh / app->camera.size.y) * app->framebufferSize.y;

	RectBorders((int)sx, (int)sy, (int)sw, (int)sh, thickness, color);
}

void UI::Image(const Texture2D& t, float x, float y, float w, float h, uint32 tint, int sort)
{
	if (noRender) return;
	Sprite s{ &t, x,y,w,h, 0,0,1,1, tint, RenderLayer::UI, sort };
	app->renderer->Queue(s);
}

void UI::Image(const Texture2D& t, float x, float y, float w, float h,
	const AtlasRectPx& srcPx, uint32 tint, int sort)
{
	if (noRender) return;

	const UVRect uv = UVFromPx(t, srcPx);
	Sprite s{};
	s.tex = &t;
	s.x = x;  s.y = y;  s.w = w;  s.h = h;
	s.u0 = uv.u0; s.v0 = uv.v0;
	s.u1 = uv.u1; s.v1 = uv.v1;
	s.color = tint;
	s.layer = RenderLayer::UI;
	s.sort = sort;

	app->renderer->Queue(s);
}

bool UI::ImageButton(const Texture2D& t, float x, float y, float w, float h, uint32 tint_rgba, uint32 tint_rgbaHover, uint32 tint_rgbaActive, int sort)
{
	bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
	RegisterHoverItem(hover, HasVisibleAlpha(tint_rgba) || HasVisibleAlpha(tint_rgbaHover) || HasVisibleAlpha(tint_rgbaActive));

	uint32 cc = hover ? (md ? tint_rgbaActive : tint_rgbaHover) : tint_rgba;
	bool clicked = hover && !md && mdPrev;
	if (hover && (md || clicked)) {
		mouseConsumed = true;
	}
	if (noRender) return clicked;

	Sprite s{ &t, x,y,w,h, 0,0,1,1, cc, RenderLayer::UI, sort  };
	app->renderer->Queue(s);

	return false;
}

bool UI::ImageButton(const Texture2D& t, float x, float y, float w, float h, const AtlasRectPx& srcPx, uint32 tint_rgba, uint32 tint_rgbaHover, uint32 tint_rgbaActive, int sort)
{
	bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
	RegisterHoverItem(hover, HasVisibleAlpha(tint_rgba) || HasVisibleAlpha(tint_rgbaHover) || HasVisibleAlpha(tint_rgbaActive));

	uint32 cc = hover ? (md ? tint_rgbaActive : tint_rgbaHover) : tint_rgba;
	bool clicked = hover && !md && mdPrev;
	if (hover && (md || clicked)) {
		mouseConsumed = true;
	}
	if (noRender) return clicked;

	Image(t, x, y, w, h, srcPx, cc, sort);
	return false;
}

bool UI::ImageButton(const Texture2D& t, float x, float y, float w, float h, const AtlasRectPx& srcPx, const AtlasRectPx& srcPxHover, const AtlasRectPx& srcPxActive, uint32 tint_rgba, int sort)
{
	bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
	RegisterHoverItem(hover, HasVisibleAlpha(tint_rgba));

	AtlasRectPx rect = hover ? (md ? srcPxActive : srcPxHover) : srcPx;
	bool clicked = hover && !md && mdPrev;
	if (hover && (md || clicked)) {
		mouseConsumed = true;
	}
	if (noRender) return clicked;

	Image(t, x, y, w, h, rect, tint_rgba, sort);
	return false;
}

void UI::ImageWorld(const Texture2D& t, float wx, float wy, float ww, float wh, const AtlasRectPx& srcPx, uint32 tint, RenderLayer layer, int sort)
{
	if (!noRender) return;

	float sx, sy, sw, sh;
	if (!app->engine->WorldRectToScreen(wx, wy, ww, wh, app->framebufferSize.x, app->framebufferSize.y, sx, sy, sw, sh)) return;

	const UVRect uv = UVFromPx(t, srcPx);

	Sprite s{};
	s.tex = &t;
	s.x = sx;
	s.y = sy;
	s.w = sw;
	s.h = sh;
	s.u0 = uv.u0;
	s.v0 = uv.v0;
	s.u1 = uv.u1;
	s.v1 = uv.v1;
	s.color = tint;
	s.layer = layer;
	s.sort = sort;

	app->renderer->Queue(s);
}



bool UI::Button(float x, float y, float w, float h,
	uint32 c, uint32 cH, uint32 cA) {
	bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
	RegisterHoverItem(hover, HasVisibleAlpha(c) || HasVisibleAlpha(cH) || HasVisibleAlpha(cA));
	uint32 cc = hover ? (md ? cA : cH) : c;
	Rect(x, y, w, h, cc);

	bool clicked = hover && !md && mdPrev;
	if (hover && (md || clicked)) {
		mouseConsumed = true;
	}
	return noRender ? clicked : false;
}




bool UI::Slider(float x, float y, float w, float h,
	float minv, float maxv, float& v,
	uint32 track, uint32 knob)
{
	float t, kx, kx0, kw;
	bool released = SliderLogic(x, y, w, h, minv, maxv, v, t, kx, kx0, kw);

	Rect(x, y + h * 0.4f, w, h * 0.2f, track);
	Rect(kx0, y, kw, h, knob);

	return released;
}

bool UI::Slider(float x, float y, float w, float h, float minv, float maxv, float& v, bool fill, uint32 tint)
{
	float t, kx, kx0, kw;
	bool released = SliderLogic(x, y, w, h, minv, maxv, v, t, kx, kx0, kw);

	//Background
	Image(interfaceTex, x, y, w, h, sliderBackgroundRect, tint, 3);

	//Fill
	float fillW = kx - x;
	if (fillW > 0.0f) {
		AtlasRectPx fillRect = sliderFillRect;
		fillRect.w = (fillW*sliderFillRect.w)/w;
		Image(interfaceTex, x, y, fillW, h, fillRect, tint, 4);
	}

	//Knob
	Image(interfaceTex, kx0, y-10, kw, h+20, sliderKnobRect, tint, 5);

	return released;
}

void UI::Circle(float cx, float cy, float r, uint32 c, int segments) {
	if (noRender) return;

	float rx = r * (float)app->framebufferSize.x / (float)app->camera.size.x;
	float ry = r * (float)app->framebufferSize.y / (float)app->camera.size.y;

	if (rx <= 0 || ry <= 0) return;

	int n = segments > 0 ? segments : std::fmax(12, std::fmin(128, int((6.2831853f * std::fmax(rx, ry)) / 8.0f)));
	const float dA = 6.2831853f / n;

	verts.reserve(verts.size() + 3 * n);
	Vertex center{ cx, cy, c };

	float px = cx + rx, py = cy;
	for (int i = 1;i <= n;++i) {
		float a = i * dA, x = cx + std::cos(a) * rx, y = cy + std::sin(a) * ry;
		verts.push_back(center); verts.push_back({ px,py,c }); verts.push_back({ x,y,c });
		px = x; py = y;
	}
}


void UI::Ring(float cx, float cy, float r, float t, uint32 c, int segments) {
	if (noRender) return;

	float vw = (float)app->framebufferSize.x;
	float vh = (float)app->framebufferSize.y;
	float cw = app->camera.size.x, ch = app->camera.size.y;

	float sxCell = floor(vw / cw);
	float syCell = floor(vh / ch);
	float s = std::fmax(1.0f, std::fmin(sxCell, syCell));

	float rp = r * s;

	if (t <= 0 || rp <= 0) return;

	float rx0 = std::fmax(0.0f, rp - t * 0.5f), ry0 = std::fmax(0.0f, rp - t * 0.5f);
	float rx1 = rp + t * 0.5f, ry1 = rp + t * 0.5f;
	int n = segments > 0 ? segments : std::fmax(12, std::fmin(128, int((6.2831853f * std::fmax(rp, rp)) / 8.0f)));
	float dA = 6.2831853f / n;
	float p0x = cx + rx0, p0y = cy, p1x = cx + rx1, p1y = cy;
	for (int i = 1;i <= n;++i) {
		float a = i * dA;
		float x0 = cx + std::cos(a) * rx0, y0 = cy + std::sin(a) * ry0;
		float x1 = cx + std::cos(a) * rx1, y1 = cy + std::sin(a) * ry1;
		verts.push_back({ p0x,p0y,c }); verts.push_back({ p1x,p1y,c }); verts.push_back({ x1,y1,c });
		verts.push_back({ p0x,p0y,c }); verts.push_back({ x1,y1,c });   verts.push_back({ x0,y0,c });
		p0x = x0; p0y = y0; p1x = x1; p1y = y1;
	}
}


void UI::Star(float cx, float cy, float r, uint32 c)
{
    if (noRender) return;
    if (r <= 0.0f) return;

    constexpr int spikes = 5;
    const float outer = r;
    const float inner = r * 0.5f;
    const float PI = 3.1415926535f;

    float ang = -PI * 0.5f;
    const float step = PI / (float)spikes;

    float px[spikes * 2];
    float py[spikes * 2];

    for (int i = 0; i < spikes * 2; ++i) {
        const float rr = (i % 2 == 0) ? outer : inner;
        px[i] = cx + std::cos(ang) * rr;
        py[i] = cy + std::sin(ang) * rr;
        ang += step;
    }

    // Triangle fan
    for (int i = 0; i < spikes * 2; ++i) {
        const int j = (i + 1) % (spikes * 2);
        PushTri(verts, cx, cy, px[i], py[i], px[j], py[j], c);
    }
}

void UI::StarOutline(float cx, float cy, float r, uint32 outline, uint32 fill)
{
    if (noRender) return;
    Star(cx, cy, r, outline);
    Star(cx, cy, r * 0.70f, fill);
}


void UI::Text(float x, float y, const char* text, uint32 rgba, float scale)
{
	if (noRender) return;
	if (!text || !*text) return;

	if (fontReady) {
		float s = std::fmax(0.01f, scale);
		
		float penX = 0.0f;
		float penY = font.AscentPx(); 
		const float line = font.LineHeightPx();

		auto utf8Next = [](const char*& p) -> uint32_t {
			const unsigned char c0 = (unsigned char)*p++;
			if (c0 < 0x80) return c0;
			if ((c0 >> 5) == 0x6) { // 110xxxxx
				const unsigned char c1 = (unsigned char)*p;
				if ((c1 & 0xC0) != 0x80) return '?';
				++p;
				return ((uint32_t)(c0 & 0x1F) << 6) | (uint32_t)(c1 & 0x3F);
			}
			if ((c0 >> 4) == 0xE) { // 1110xxxx
				const unsigned char c1 = (unsigned char)p[0];
				const unsigned char c2 = (unsigned char)p[1];
				if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return '?';
				p += 2;
				return ((uint32_t)(c0 & 0x0F) << 12) | ((uint32_t)(c1 & 0x3F) << 6) | (uint32_t)(c2 & 0x3F);
			}
			if ((c0 >> 3) == 0x1E) { // 11110xxx
				const unsigned char c1 = (unsigned char)p[0];
				const unsigned char c2 = (unsigned char)p[1];
				const unsigned char c3 = (unsigned char)p[2];
				if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return '?';
				p += 3;
				return ((uint32_t)(c0 & 0x07) << 18) | ((uint32_t)(c1 & 0x3F) << 12) | ((uint32_t)(c2 & 0x3F) << 6) | (uint32_t)(c3 & 0x3F);
			}
			return '?';
		};

		for (const char* p = text; *p;) {
			uint32_t cp = utf8Next(p);
			if (cp == '\n') {
				penY += line;
				penX = 0.0f;
				continue;
			}
			if (cp == '\t') {
				const int spaceIdx = font.GlyphIndex(' ');
				if (spaceIdx >= 0) penX += font.Baked()[spaceIdx].xadvance * 4.0f;
				continue;
			}

			int idx = font.GlyphIndex(cp);
			if (idx < 0) idx = font.GlyphIndex('?');
			if (idx < 0) continue;

			float bx = penX;
			float by = penY;
			stbtt_aligned_quad q{};
			stbtt_GetBakedQuad(font.Baked(), font.AtlasW(), font.AtlasH(), idx, &bx, &by, &q, 1);
			penX = bx;
			penY = by;

			const float x0 = std::floor(x + q.x0 * s);
			const float y0 = std::floor(y + q.y0 * s);
			const float x1 = std::floor(x + q.x1 * s);
			const float y1 = std::floor(y + q.y1 * s);

			const float w = x1 - x0;
			const float h = y1 - y0;
			if (w <= 0.0f || h <= 0.0f) continue;

			Sprite spr;
			spr.tex = &font.Atlas();
			spr.x = x0;
			spr.y = y0;
			spr.w = w;
			spr.h = h;
			spr.u0 = q.s0;
			spr.v0 = q.t0;
			spr.u1 = q.s1;
			spr.v1 = q.t1;
			spr.color = rgba;
			spr.layer = RenderLayer::UI;
			spr.sort = 10;
			app->renderer->Queue(spr);
		}
		return;
	}
}

void UI::TextCentered(float x, float y, float w, float h, const char* text, uint32 rgba, float scale)
{
	if (noRender) return;
	if (!text || !*text) return;

	if (fontReady) {
		float s = std::fmax(0.01f, scale);
		float line = font.LineHeightPx() * s;

		auto utf8Next = [](const char*& p) -> uint32_t {
			const unsigned char c0 = (unsigned char)*p++;
			if (c0 < 0x80) return c0;
			if ((c0 >> 5) == 0x6) { const unsigned char c1 = (unsigned char)*p; if ((c1 & 0xC0) != 0x80) return '?'; ++p; return ((uint32_t)(c0 & 0x1F) << 6) | (uint32_t)(c1 & 0x3F); }
			if ((c0 >> 4) == 0xE) { const unsigned char c1 = (unsigned char)p[0]; const unsigned char c2 = (unsigned char)p[1]; if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return '?'; p += 2; return ((uint32_t)(c0 & 0x0F) << 12) | ((uint32_t)(c1 & 0x3F) << 6) | (uint32_t)(c2 & 0x3F); }
			if ((c0 >> 3) == 0x1E) { const unsigned char c1 = (unsigned char)p[0]; const unsigned char c2 = (unsigned char)p[1]; const unsigned char c3 = (unsigned char)p[2]; if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return '?'; p += 3; return ((uint32_t)(c0 & 0x07) << 18) | ((uint32_t)(c1 & 0x3F) << 12) | ((uint32_t)(c2 & 0x3F) << 6) | (uint32_t)(c3 & 0x3F); }
			return '?';
		};

		float cur = 0.0f;
		float maxW = 0.0f;
		int lines = 1;
		for (const char* p = text; *p;) {
			uint32_t cp = utf8Next(p);
			if (cp == '\n') { maxW = std::fmax(maxW, cur); cur = 0.0f; ++lines; continue; }
			if (cp == '\t') {
				const int spaceIdx = font.GlyphIndex(' ');
				if (spaceIdx >= 0) cur += font.Baked()[spaceIdx].xadvance * s * 4.0f;
				continue;
			}
			int idx = font.GlyphIndex(cp);
			if (idx < 0) idx = font.GlyphIndex('?');
			if (idx < 0) continue;
			cur += font.Baked()[idx].xadvance * s;
		}
		maxW = std::fmax(maxW, cur);
		float totalH = lines * line;

		float tx = x + (w - maxW) * 0.5f;
		float ty = y + (h - totalH) * 0.5f;
		Text(tx, ty, text, rgba, s);
		return;
	}
}

void UI::Cursor()
{
	Image(cursorTex, mx, my, 20, 23, RGBAu32(255, 255, 255, 255), 999);
}
