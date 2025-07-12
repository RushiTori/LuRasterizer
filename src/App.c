#include "App.h"

#define RASTER_SCALE 4

bool AppInit(App* app) {
	const uint32_t winWidth = 1920 / 2;
	const uint32_t winHeight = 1080 / 2;

	const uint32_t rasterWidth = winWidth / RASTER_SCALE;
	const uint32_t rasterHeight = winHeight / RASTER_SCALE;

	SetTraceLogLevel(LOG_WARNING);
	SetTargetFPS(60);
	InitWindow(winWidth, winHeight, "LuRaster - made with Raylib in C");

	app->rasterTarget = CreateRasterTarget(rasterWidth, rasterHeight);
	if (!app->rasterTarget) {
		CloseWindow();
		return false;
	}

	app->a = (Vector2){.x = nanf(""), .y = nanf("")};
	app->b = (Vector2){.x = nanf(""), .y = nanf("")};

	return true;
}

static bool Vector2IsNan(Vector2 v) { return (isnan(v.x) || isnan(v.y)); }

void AppUpdate(App* app) {
	Vector2 mousePos = GetMousePosition();
	mousePos = Vector2Scale(mousePos, 1.0f / RASTER_SCALE);

	RasterTargetClearBackground(app->rasterTarget, PINK);

	if (IsKeyPressed(KEY_KP_1)) app->a = mousePos;
	if (IsKeyPressed(KEY_KP_2)) app->b = mousePos;

	if (!Vector2IsNan(app->a) && !Vector2IsNan(app->b)) RasterTargetDrawTriangle(app->rasterTarget, app->a, app->b, mousePos, RED);
	if (!Vector2IsNan(app->a)) RasterTargetDrawPixel(app->rasterTarget, app->a.x, app->a.y, BLUE);
	if (!Vector2IsNan(app->b)) RasterTargetDrawPixel(app->rasterTarget, app->b.x, app->b.y, BLUE);
	RasterTargetDrawPixel(app->rasterTarget, mousePos.x, mousePos.y, BLUE);

	RasterTargetUpdateTexture(app->rasterTarget);
}

void AppRender(const App* app) {
	BeginDrawing();
	ClearBackground(BLACK);

	RasterTargetRenderTextureEx(app->rasterTarget, RASTER_SCALE);

	EndDrawing();
}

void AppClose(App* app) {
	FreeRasterTarget(app->rasterTarget);
	CloseWindow();
}
