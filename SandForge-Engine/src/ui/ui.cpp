#include "ui/ui.h" 
#include <glad/gl.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "app/module.h"  
#include "app/app.h"
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
	fontReady = font.Load("assets/fonts/PixeloidSans.ttf", 24.0f, 1024, 1024, true);
	matAtlasReady = matAtlas.Load(SPRITE_DIR "/materialAtlas.png");
	curorTexReady = cursorTex.Load(SPRITE_DIR "/cursor.png");
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
	matAtlasReady = false;


	if (vbo) glDeleteBuffers(1, &vbo);
	if (vao) glDeleteVertexArrays(1, &vao);
	if (prog) glDeleteProgram(prog);
	vbo = vao = prog = 0;

	return true;

}

void UI::Begin(int viewW, int viewH) {
	vw = viewW; vh = viewH; verts.clear();
}

void UI::Draw(int& brushSize, Material& brushMat) {

	const MatProps actualMat = matProps((uint8)brushMat);


	float pad = 8.0f, y = 8.0f, x = 550.0f, btn = 28.0f;

	x += 12.0f; float bx = x, bw = 200.0f, bh = 20.0f; float v = (float)brushSize;
	Slider(bx, y + 4.0f, bw, bh, 1.0f, 64.0f, v, RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));
	brushSize = (int)(v + 0.5f);

	Ring(mx, my, brushSize, 2, RGBAu32(actualMat.color.r, actualMat.color.g, actualMat.color.b, 100), 12);

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

	if (app->showGridBounds) {
		RectBordersWorld(0, 0, app->gridSize.x, app->gridSize.y, 2.0f, RGBAu32(120, 220, 255, 200));
	}

	if (app->shiftPressed && !md) {
		Rect(mx-(app->framebufferSize.x * 1.5f), my, app->framebufferSize.x*3, 1, RGBAu32(220, 220, 220, 100));
		Rect(mx, my-(app->framebufferSize.y * 1.5f), 1, app->framebufferSize.y * 3, RGBAu32(220, 220, 220, 100));
	}
}

void UI::End() {

	if (noRender) return;
	Flush();
}

void UI::SetMouse(double x, double y, bool down) {
	mdPrev = md; md = down; mx = x; my = y; mouseConsumed = false;
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

bool UI::ImageButton(const Texture2D& t, float x, float y, float w, float h, uint32 tint_rgba, uint32 tint_rgbaHover, uint32 tint_rgbaActive, int sort)
{
	bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);

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



bool UI::Button(float x, float y, float w, float h,
	uint32 c, uint32 cH, uint32 cA) {
	bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
	uint32 cc = hover ? (md ? cA : cH) : c;
	Rect(x, y, w, h, cc);

	bool clicked = hover && !md && mdPrev;
	if (hover && (md || clicked)) {
		mouseConsumed = true;
	}
	return noRender ? clicked : false;
}




bool UI::ButtonAtlas(float x, float y, float w, float h,
    int atlasIndex, uint32 c, uint32 cH, uint32 cA)
{
    const bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
    const uint32 bg = hover ? (md ? cA : cH) : c;
    const bool clicked = hover && !md && mdPrev;

    if (hover && (md || clicked)) mouseConsumed = true;

    // Render pass only
    if (!noRender) {
        if (matAtlasReady && atlasIndex >= 0 && atlasIndex < 10) {
            // Fondo
            const UVRect white = WhitePixelUV(matAtlas);
            Sprite bgS{};
            bgS.tex = &matAtlas;
            bgS.x = x; bgS.y = y; bgS.w = w; bgS.h = h;
            bgS.u0 = white.u0; bgS.v0 = white.v0;
            bgS.u1 = white.u1; bgS.v1 = white.v1;
            bgS.color = bg;
            bgS.layer = RenderLayer::UI;
            app->renderer->Queue(bgS);

            // Icono
            const UVRect uv = UVFromPx(matAtlas, kMatAtlasPx[atlasIndex]);
            const float iw = w * 0.75f;
            const float ih = h * 0.75f;
            const float ix = x + (w - iw) * 0.5f;
            const float iy = y + (h - ih) * 0.5f;

            Sprite ic{};
            ic.tex = &matAtlas;
            ic.x = ix; ic.y = iy; ic.w = iw; ic.h = ih;
            ic.u0 = uv.u0; ic.v0 = uv.v0; ic.u1 = uv.u1; ic.v1 = uv.v1;
            ic.color = 0xFFFFFFFF;
            ic.layer = RenderLayer::UI;
            app->renderer->Queue(ic);
        }
        else {
            // Fallback: rect simple si no hay atlas/índice válido
            Rect(x, y, w, h, bg);
        }
    }

    return noRender ? clicked : false;
}

bool UI::Slider(float x, float y, float w, float h,
	float minv, float maxv, float& v,
	uint32 track, uint32 knob) {
	if (v < minv) v = minv; if (v > maxv) v = maxv;
	Rect(x, y + h * 0.4f, w, h * 0.2f, track);
	float t = (v - minv) / (maxv - minv);
	float kx = x + t * w;
	float kw = h;
	float kx0 = kx - kw * 0.5f;
	Rect(kx0, y, kw, h, knob);


	bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
	if (noRender) {
		if (hover && md) { v = minv + float((mx - x) / w) * (maxv - minv); mouseConsumed = true; }
		return hover && !md && mdPrev; // soltado sobre el slider
	}
	return false;
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

			const float w = (q.x1 - q.x0) * s;
			const float h = (q.y1 - q.y0) * s;
			if (w <= 0.0f || h <= 0.0f) continue;

			Sprite spr;
			spr.tex = &font.Atlas();
			spr.x = x + q.x0 * s;
			spr.y = y + q.y0 * s;
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
