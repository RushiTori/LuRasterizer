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

	app->rasterTarget = RasterTargetCreate(rasterWidth, rasterHeight);
	if (!app->rasterTarget) {
		CloseWindow();
		return false;
	}

	app->cubeModel = LoadRasterModelFromFile("models/cube.obj");
	if (!app->cubeModel) {
		RasterTargetFree(app->rasterTarget);
		CloseWindow();
		return false;
	}

	return true;
}

void AppUpdate(App* app) {
	RasterTargetClearBackground(app->rasterTarget, PINK);

	srand(1);  // So that the cube always has the same colors
	RasterTargetDrawModel(app->rasterTarget, app->cubeModel);

	RasterTargetUpdateTexture(app->rasterTarget);
}

void AppRender(const App* app) {
	BeginDrawing();
	ClearBackground(BLACK);

	RasterTargetRenderTextureEx(app->rasterTarget, RASTER_SCALE);

	EndDrawing();
}

void AppClose(App* app) {
	RasterTargetFree(app->rasterTarget);
	CloseWindow();
}
