#ifndef RASTER_TARGET_H
#define RASTER_TARGET_H

#include <LuLib/LuFile.h>
#include <LuLib/LuMemSafety.h>
#include <raylib.h>

#undef Clamp
#undef Lerp

#include <raymath.h>

typedef struct RasterTarget {
	Color* pixels;
	uint32_t width;
	uint32_t height;

	Texture tex;
} RasterTarget;

RasterTarget* CreateRasterTarget(uint32_t width, uint32_t height);
void FreeRasterTarget(RasterTarget* screen);

bool RasterTargetSaveToFile(const RasterTarget* screen, const char* path);

void RasterTargetUpdateTexture(RasterTarget* screen);
void RasterTargetRenderTexture(const RasterTarget* screen);
void RasterTargetRenderTextureEx(const RasterTarget* screen, uint32_t scale);

void RasterTargetClearBackground(RasterTarget* screen, Color col);
void RasterTargetDrawPixel(RasterTarget* screen, uint32_t x, uint32_t y, Color col);
void RasterTargetDrawTriangle(RasterTarget* screen, Vector2 a, Vector2 b, Vector2 c, Color col);

#endif	// RASTER_TARGET_H
