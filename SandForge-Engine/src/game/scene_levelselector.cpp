#include "scene_levelselector.h"
#include "scene_manager.h"
#include "app/app.h"
#include "core/input.h"
#include "ui/ui.h"
#include <cmath>
#include <cstdio>

void Scene_LevelSelector::Update(float dt)
{
	(void)dt;
	if (app->input->KeyDown(GLFW_KEY_ESCAPE)) mgr->Request(SCENE_MAINMENU);
}

void Scene_LevelSelector::DrawUI(int&, Material&)
{
	UI* ui = app->ui;
	const float vw = (float)app->framebufferSize.x;
	const float vh = (float)app->framebufferSize.y;

	ui->Cursor();

	// Escalado simple en base a 1280x720
	float s = std::fmin(vw / 1280.0f, vh / 720.0f);
	if (s < 0.85f) s = 0.85f;
	if (s > 1.25f) s = 1.25f;
	auto S = [&](float v) { return v * s; };

	// --------------------------------------------------------
	// Fondo + barra superior
	// --------------------------------------------------------
	const uint32 bg = RGBAu32(50, 47, 44, 255);
	const uint32 top = RGBAu32(38, 36, 33, 255);

	const float topH = S(76.0f);
	ui->Rect(0, 0, vw, vh, bg);
	ui->Rect(0, 0, vw, topH, top);
	ui->Rect(0, topH, vw, S(2.0f), RGBAu32(40, 32, 25, 180));

	ui->TextCentered(0, S(12.0f), vw, S(44.0f), "LEVEL SELECT",
		RGBAu32(245, 245, 250, 245), 1.15f * s);

	// --------------------------------------------------------
	// Botones arriba (Back / Settings)
	// --------------------------------------------------------
	{
		float bx = S(16.0f), by = S(22.0f), bw = S(94.0f), bh = S(32.0f);
		uint32 c = RGBAu32(110, 70, 70, 220);
		if (ui->Button(bx, by, bw, bh, c, MulRGBA(c, 1.15f), MulRGBA(c, 0.85f)))
			mgr->Request(SCENE_MAINMENU);
		ui->TextCentered(bx, by, bw, bh, "BACK", RGBAu32(250, 250, 250, 240), 0.85f * s);
	}

	{
		float bw = S(94.0f), bh = S(32.0f);
		float bx = vw - S(16.0f) - bw, by = S(22.0f);
		uint32 c = RGBAu32(70, 90, 140, 220);
		if (ui->Button(bx, by, bw, bh, c, MulRGBA(c, 1.15f), MulRGBA(c, 0.85f)))
			mgr->OpenSettings(SCENE_LEVELSELECTOR);
		ui->TextCentered(bx, by, bw, bh, "SETTINGS", RGBAu32(250, 250, 250, 240), 0.85f * s);
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

		const float starSize = S(18.0f);
		const float sx = pillX + S(12.0f);
		const float sy = pillY + (pillH - starSize) * 0.5f;

		
		ui->Image(ui->interfaceTex, sx, sy, starSize, starSize, ui->starIconRect, RGBAu32(250, 210, 70, 240));
		

		ui->Text(pillX + S(36.0f), pillY + S(2.0f), buf, RGBAu32(240, 240, 240, 240), 1.0f * s);
	}

	// --- Thumbnails
	constexpr int LEVEL_COUNT = 15;
	static Texture2D sThumb[LEVEL_COUNT];
	static bool sThumbTried[LEVEL_COUNT] = { false };

	// Grid 5x3
	constexpr int cols = 5;
	constexpr int rows = 3;

	// Espaciados
	const float padX = S(52.0f);
	const float padTop = pillY + pillH + S(18.0f);
	const float padBottom = S(40.0f);
	const float gap = S(16.0f);

	const float gridW = vw - padX * 2.0f;
	const float gridH = vh - padTop - padBottom;
	const float tileW = (gridW - gap * (cols - 1)) / cols;
	const float tileH = (gridH - gap * (rows - 1)) / rows;

	//Mouse for hover and click on images
	const SceneId levelScenes[LEVEL_COUNT] = {
		SCENE_LEVEL1, SCENE_LEVEL2, SCENE_LEVEL3, SCENE_LEVEL4, SCENE_LEVEL5,
		SCENE_LEVEL6, SCENE_LEVEL7, SCENE_LEVEL8, SCENE_LEVEL9, SCENE_LEVEL10,
		SCENE_LEVEL11, SCENE_LEVEL12, SCENE_LEVEL13, SCENE_LEVEL14, SCENE_LEVEL15
	};

	for (int i = 0; i < LEVEL_COUNT; ++i) {
		const int rr = i / cols;
		const int cc = i % cols;

		const float x = padX + cc * (tileW + gap);
		const float y = padTop + rr * (tileH + gap);

		const bool unlocked = app->progress.IsLevelUnlocked(i);

		// Panel image
		
		uint32 tint = unlocked ? RGBAu32(235, 235, 235, 255) : RGBAu32(255, 255, 255, 90);
		uint32 hover = unlocked ? RGBAu32(255, 255, 255, 255) : RGBAu32(255, 255, 255, 90);
		uint32 active = unlocked ? RGBAu32(210, 210, 210, 255) : RGBAu32(255, 255, 255, 90);
		const bool clicked = ui->ImageButton(ui->interfaceTex, x, y, tileW, tileH, ui->panelLevelRect, tint, hover, active);
		if (clicked && unlocked) mgr->Request(levelScenes[i]);
			

		//Thumb panel
		if (!sThumbTried[i]) {
			sThumbTried[i] = true;
			char p[128];
			std::snprintf(p, sizeof(p), "assets/sprites/level_thumbs/level_%02d.png", i + 1);
			sThumb[i].Load(p);
		}
		const float p = S(10.0f);
		float frameX = x + p;
		float frameY = y + p;
		float frameW = tileW - p * 2.0f;
		float frameH = frameW * (9.0f / 16.0f);

		const float reservedBelow = S(56.0f);
		const float maxFrameH = tileH - p - reservedBelow;
		if (frameH > maxFrameH) frameH = maxFrameH;
		if (frameH < S(8.0f)) frameH = S(8.0f);

		
		tint = unlocked ? RGBAu32(255, 255, 255, 255) : RGBAu32(255, 255, 255, 190);
		ui->Image(ui->interfaceTex, frameX, frameY, frameW, frameH, ui->thumbLevelRect, tint);
		

		const float inner = S(6.0f);
		const float tx = frameX + inner;
		const float ty = frameY + inner;
		const float tw = frameW - inner * 2.0f;
		const float th = frameH - inner * 2.0f;

		// Thumbnail image
		if (sThumb[i].id != 0) {
			const uint32 tint = unlocked ? RGBAu32(255, 255, 255, 255) : RGBAu32(200, 200, 200, 130);
			ui->Image(sThumb[i], tx, ty, tw, th, tint);
		}

		//Title
		char name[32];
		std::snprintf(name, sizeof(name), "LEVEL %02d", i + 1);

		const float nameY = frameY + frameH + S(5.0f);
		ui->Text(x + S(12.0f), nameY, name,
			unlocked ? RGBAu32(240, 240, 245, 235) : RGBAu32(185, 185, 195, 170),
			unlocked ? 0.90f * s : 0.60f * s);

		// Stars
		if (unlocked) {
			const uint8 earned = app->progress.StarsFor(i);

			const float starSize = S(18.0f);
			const float stepX = S(22.0f);
			const float marginR = S(14.0f);

			const float starY = y + tileH - S(10.0f) - starSize;
			const float startX = x + tileW - marginR - starSize - stepX * 2.0f;

			for (int st = 0; st < 3; ++st) {
				const float sx = startX + st * stepX;
				if (st < (int)earned) {
					ui->Image(ui->interfaceTex, sx, starY, starSize, starSize, ui->starIconRect, RGBAu32(250, 210, 70, 240));
				}
				else {
					ui->Image(ui->interfaceTex, sx, starY, starSize, starSize, ui->starEmptyIconRect, RGBAu32(220, 220, 230, 200));
				}
			}
		}

		// Locked
		if (!unlocked) {

			//Candado
			
			const float lockS = std::fmin(frameW, frameH) * 0.80f;
			const float lx = frameX + (frameW - lockS) * 0.5f; 
			const float ly = frameY + (frameH - lockS) * 0.5f;
			ui->Image(ui->interfaceTex, lx, ly, lockS, lockS, ui->lockIconRect,RGBAu32(255, 255, 255, 210), 1);
			

			//Pill need stars
			char req[32];
			std::snprintf(req, sizeof(req), "NEED %d STARS", app->progress.UnlockRequirement(i));

			const float rw = tileW * 0.78f;
			const float rh = S(22.0f);
			const float rx = x + (tileW - rw) * 0.5f;
			const float ry = y + tileH - rh - S(10.0f);

			ui->Rect(rx, ry, rw, rh, RGBAu32(20, 20, 26, 235));
			ui->RectBorders(rx, ry, rw, rh, S(2.0f), RGBAu32(120, 120, 140, 80));
			ui->TextCentered(rx, ry, rw, rh, req, RGBAu32(240, 240, 245, 225), 0.72f * s);
		}
	}
}
