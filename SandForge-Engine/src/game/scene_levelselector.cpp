#include "scene_levelselector.h"
#include "scene_manager.h"
#include "app/app.h"
#include "core/input.h"
#include "ui/ui.h"
#include "ui/ui_anim.h"
#include "render/texture.h"
#include <cmath>
#include <cstdio>

void Scene_LevelSelector::OnEnter()
{
	uiIntroTimer = 0.0f;
	backHoverT = 0.0f;
	settingsHoverT = 0.0f;
	levelHoverT.fill(0.0f);
}

void Scene_LevelSelector::Update(float dt)
{
	uiIntroTimer += dt;
	if (app->input->KeyDown(GLFW_KEY_ESCAPE)) mgr->Request(SCENE_MAINMENU);
}

bool Scene_LevelSelector::DrawTopButton(float x, float y, float w, float h, const char* text, uint32 color, float delay, float introDir, float& hoverT)
{
	UI* ui = app->ui;

	float mx = (float)app->input->MouseX();
	float my = (float)app->input->MouseY();

	float introAlpha = UIAnim::IntroAlpha(uiIntroTimer, delay, 0.26f);
	float introMove = UIAnim::IntroMove(uiIntroTimer, delay, 0.34f);
	float bx = x + (1.0f - introMove) * introDir * 72.0f;

	bool hover = (mx >= bx && mx <= bx + w && my >= y && my <= y + h);
	hoverT = UIAnim::Hover(hoverT, hover, 8.0f, app->dt);

	float scale = 1.0f + hoverT * 0.08f;
	float drawW = w * scale;
	float drawH = h * scale;
	float drawX = bx - (drawW - w) * 0.5f;
	float drawY = y - (drawH - h) * 0.5f - hoverT * 2.0f;

	if (ui->ImageButton(ui->interfaceTex, drawX, drawY, drawW, drawH, ui->buttonUp, ui->buttonDown, ui->buttonDown, UIAnim::AlphaRGBA(color, introAlpha)))
		return true;

	ui->TextCentered(drawX + 2, drawY - 2, drawW, drawH, text, RGBAu32(250, 250, 250, (unsigned)(240.0f * introAlpha)), 0.75f + hoverT * 0.03f);
	return false;
}

void Scene_LevelSelector::DrawUI(int&, Material&)
{
	UI* ui = app->ui;
	float vw = (float)app->framebufferSize.x;
	float vh = (float)app->framebufferSize.y;

	ui->Cursor();

	// Escalado simple en base a 1280x720
	float s = std::fmin(vw / 1280.0f, vh / 720.0f);
	if (s < 0.85f) s = 0.85f;
	if (s > 1.25f) s = 1.25f;
	auto S = [&](float v) { return v * s; };

	// --------------------------------------------------------
	// Fondo + barra superior
	// --------------------------------------------------------
	uint32 bg = RGBAu32(50, 47, 44, 255);
	uint32 top = RGBAu32(38, 36, 33, 255);

	float topH = S(76.0f);
	ui->Rect(0, 0, vw, vh, bg);
	ui->Rect(0, 0, vw, topH, top);
	ui->Rect(0, topH, vw, S(2.0f), RGBAu32(40, 32, 25, 180));

	ui->TextCentered(0, S(12.0f), vw, S(44.0f), "LEVEL SELECT", RGBAu32(245, 245, 250, 245), 1.15f * s);

	// --------------------------------------------------------
	// Botones arriba (Back / Settings)
	// --------------------------------------------------------
	{
		float bx = S(16.0f), by = S(22.0f), bw = S(100.0f), bh = S(42.0f);
		if (DrawTopButton(bx, by, bw, bh, "BACK", RGBAu32(220, 170, 170, 220), 0.06f, -1.0f, backHoverT))
			mgr->Request(SCENE_MAINMENU);
	}

	{
		float bw = S(100.0f), bh = S(42.0f);
		float bx = vw - S(16.0f) - bw, by = S(22.0f);
		if (DrawTopButton(bx, by, bw, bh, "SETTINGS", RGBAu32(102, 161, 255, 220), 0.14f, 1.0f, settingsHoverT))
			mgr->OpenSettings(SCENE_LEVELSELECTOR);
	}

	// --------------------------------------------------------
	// Total estrellas (pill centrada)
	// --------------------------------------------------------
	float pillW = S(70.0f), pillH = S(30.0f);
	float pillX = vw * 0.5f - pillW * 0.5f;
	float pillY = topH + S(14.0f);

	ui->Rect(pillX, pillY, pillW, pillH, RGBAu32(38, 36, 33, 255));
	ui->RectBorders(pillX, pillY, pillW, pillH, S(2.0f), RGBAu32(40, 32, 25, 180));

	{
		char buf[32];
		std::snprintf(buf, sizeof(buf), "%d", app->progress.TotalStars());

		float starSize = S(18.0f);
		float sx = pillX + S(12.0f);
		float sy = pillY + (pillH - starSize) * 0.5f;

		ui->Image(ui->interfaceTex, sx, sy, starSize, starSize, ui->starIconRect, RGBAu32(250, 210, 70, 240));
		ui->Text(pillX + S(36.0f), pillY + S(2.0f), buf, RGBAu32(240, 240, 240, 240), 1.0f * s);
	}

	// --- Thumbnails
	constexpr int LEVEL_COUNT = 12;
	static Texture2D sThumb[LEVEL_COUNT];
	static bool sThumbTried[LEVEL_COUNT] = { false };

	// Grid 5x3
	constexpr int cols = 4;
	constexpr int rows = 3;

	// Espaciados
	float padX = S(52.0f);
	float padTop = pillY + pillH + S(18.0f);
	float padBottom = S(40.0f);
	float gap = S(16.0f);

	float gridW = vw - padX * 2.0f;
	float gridH = vh - padTop - padBottom;
	float tileW = (gridW - gap * (cols - 1)) / cols;
	float tileH = (gridH - gap * (rows - 1)) / rows;

	const SceneId levelScenes[LEVEL_COUNT] = {
		SCENE_LEVEL1, SCENE_LEVEL2, SCENE_LEVEL3, SCENE_LEVEL4,
		SCENE_LEVEL5, SCENE_LEVEL6, SCENE_LEVEL7, SCENE_LEVEL8,
		SCENE_LEVEL9, SCENE_LEVEL10, SCENE_LEVEL11, SCENE_LEVEL12
	};

	for (int i = 0; i < LEVEL_COUNT; ++i) {
		int rr = i / cols;
		int cc = i % cols;

		float x = padX + cc * (tileW + gap);
		float y = padTop + rr * (tileH + gap);

		float introDelay = 0.22f + 0.06f * i;
		float introAlpha = UIAnim::IntroAlpha(uiIntroTimer, introDelay, 0.26f);
		float introMove = UIAnim::IntroMove(uiIntroTimer, introDelay, 0.36f);
		float iy = y + (1.0f - introMove) * 38.0f * s;

		float mx = (float)app->input->MouseX();
		float my = (float)app->input->MouseY();
		bool hover = (mx >= x && mx <= x + tileW && my >= iy && my <= iy + tileH);
		levelHoverT[i] = UIAnim::Hover(levelHoverT[i], hover, 7.0f, app->dt);

		float hoverT = levelHoverT[i];
		float scale = 1.0f + hoverT * 0.045f;
		float drawW = tileW * scale;
		float drawH = tileH * scale;
		float drawX = x - (drawW - tileW) * 0.5f;
		float drawY = iy - (drawH - tileH) * 0.5f - hoverT * 3.0f * s;

		bool unlocked = app->progress.IsLevelUnlocked(i);
		bool special = app->progress.MaxStarsFor(i) == 1;

		// Panel image
		
		uint32 tint = unlocked
			? (special ? RGBAu32(145, 61, 235, 255) : RGBAu32(235, 235, 235, 255))
			: (special ? RGBAu32(145, 61, 235, 90) : RGBAu32(255, 255, 255, 90));
		uint32 hoverColor = unlocked
			? (special ? RGBAu32(125, 81, 255, 255) : RGBAu32(255, 255, 255, 255))
			: tint;
		uint32 active = unlocked
			? (special ? RGBAu32(135, 91, 255, 255) : RGBAu32(210, 210, 210, 255))
			: tint;

		tint = UIAnim::AlphaRGBA(tint, introAlpha);
		hoverColor = UIAnim::AlphaRGBA(hoverColor, introAlpha);
		active = UIAnim::AlphaRGBA(active, introAlpha);

		bool clicked = ui->ImageButton(ui->interfaceTex, drawX, drawY, drawW, drawH, ui->panelLevelRect, tint, hoverColor, active);
		if (clicked && unlocked) mgr->Request(levelScenes[i]);
			

		//Thumb panel
		if (!sThumbTried[i]) {
			sThumbTried[i] = true;
			char p[128];
			std::snprintf(p, sizeof(p), "assets/sprites/level_thumbs/level_%02d.png", i + 1);
			sThumb[i].Load(p);
		}

		float p = S(10.0f);
		float frameX = drawX + p;
		float frameY = drawY + p;
		float frameW = drawW - p * 2.0f;
		float frameH = frameW * (9.0f / 16.0f);

		float reservedBelow = S(56.0f);
		float maxFrameH = drawH - p - reservedBelow;
		if (frameH > maxFrameH) frameH = maxFrameH;
		if (frameH < S(8.0f)) frameH = S(8.0f);

		tint = unlocked ? RGBAu32(255, 255, 255, 255) : RGBAu32(255, 255, 255, 190);
		ui->Image(ui->interfaceTex, frameX, frameY, frameW, frameH, ui->thumbLevelRect, UIAnim::AlphaRGBA(tint, introAlpha));

		float inner = S(6.0f);
		float tx = frameX + inner;
		float ty = frameY + inner;
		float tw = frameW - inner * 2.0f;
		float th = frameH - inner * 2.0f;

		// Thumbnail image
		if (sThumb[i].id != 0) {
			uint32 thumbTint = unlocked ? RGBAu32(255, 255, 255, 255) : RGBAu32(200, 200, 200, 130);
			ui->Image(sThumb[i], tx, ty, tw, th, UIAnim::AlphaRGBA(thumbTint, introAlpha));
		}

		//Title
		char name[32];
		std::snprintf(name, sizeof(name), "LEVEL %02d", i + 1);

		float nameY = frameY + frameH + S(5.0f);
		ui->Text(drawX + S(12.0f), nameY, name,
			UIAnim::AlphaRGBA(unlocked
				? (special ? RGBAu32(245, 235, 255, 235) : RGBAu32(240, 240, 245, 235))
				: (special ? RGBAu32(200, 190, 210, 170) : RGBAu32(185, 185, 195, 170)), introAlpha),
			(unlocked ? 0.90f : 0.60f) * s + hoverT * 0.03f * s);

		// Stars
		if (unlocked) {
			uint8 earned = app->progress.StarsFor(i);
			int starSlots = app->progress.MaxStarsFor(i);

			float starSize = S(18.0f);
			float stepX = S(22.0f);
			float marginR = S(14.0f);

			float starY = drawY + drawH - S(10.0f) - starSize;
			float startX = drawX + drawW - marginR - starSize - stepX * (starSlots - 1);

			for (int st = 0; st < starSlots; ++st) {
				float sx = startX + st * stepX;
				if (st < (int)earned) {
					ui->Image(ui->interfaceTex, sx, starY, starSize, starSize, ui->starIconRect, UIAnim::AlphaRGBA(RGBAu32(250, 210, 70, 240), introAlpha));
				}
				else {
					ui->Image(ui->interfaceTex, sx, starY, starSize, starSize, ui->starEmptyIconRect, UIAnim::AlphaRGBA(RGBAu32(220, 220, 230, 200), introAlpha));
				}
			}
		}

		// Locked
		if (!unlocked) {
			// Candado
			float lockS = std::fmin(frameW, frameH) * 0.80f;
			float lx = frameX + (frameW - lockS) * 0.5f;
			float ly = frameY + (frameH - lockS) * 0.5f;
			ui->Image(ui->interfaceTex, lx, ly, lockS, lockS, ui->lockIconRect, UIAnim::AlphaRGBA(RGBAu32(255, 255, 255, 210), introAlpha), 1);

			// Pill need stars
			char req[32];
			std::snprintf(req, sizeof(req), "NEED %d STARS", app->progress.UnlockRequirement(i));

			float rw = drawW * 0.78f;
			float rh = S(22.0f);
			float rx = drawX + (drawW - rw) * 0.5f;
			float ry = drawY + drawH - rh - S(10.0f);

			ui->Rect(rx, ry, rw, rh, UIAnim::AlphaRGBA(RGBAu32(20, 20, 26, 235), introAlpha));
			ui->RectBorders(rx, ry, rw, rh, S(2.0f), UIAnim::AlphaRGBA(RGBAu32(120, 120, 140, 80), introAlpha));
			ui->TextCentered(rx, ry, rw, rh, req, UIAnim::AlphaRGBA(RGBAu32(240, 240, 245, 225), introAlpha), 0.72f * s);
		}
	}
}
