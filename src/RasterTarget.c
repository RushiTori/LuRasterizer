#include "RasterTarget.h"

static Image RasterTargetToImage(const RasterTarget* screen) {
	return (Image){
		.data = screen->pixels,
		.width = screen->width,
		.height = screen->height,
		.mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
	};
}

RasterTarget* CreateRasterTarget(uint32_t width, uint32_t height) {
	RasterTarget* screen = NULL;
	if (!Malloc(screen, sizeof(RasterTarget))) return NULL;

	if (!Malloc(screen->pixels, width * height * sizeof(Color))) FreeAndReturn(screen, NULL);
	screen->width = width;
	screen->height = height;

	screen->tex = LoadTextureFromImage(RasterTargetToImage(screen));
	if (!IsTextureValid(screen->tex)) {
		Free(screen->pixels);
		FreeAndReturn(screen, NULL);
	}

	return screen;
}

void FreeRasterTarget(RasterTarget* screen) {
	Free(screen->pixels);
	UnloadTexture(screen->tex);
	Free(screen);
}

typedef enum WriteBMPErr {
	WRITE_BMP_NO_ERR = 0,
	WRITE_BMP_FILE_ERR,
	WRITE_BMP_HEADER_ERR,
	WRITE_BMP_PIXELS_ERR,
	WRITE_BMP_PADDING_ERR,
} WriteBMPErr;

#define WriteBMPCloseAndExit(err) \
	{                             \
		CloseFile(outFile);       \
		return err;               \
	}

static WriteBMPErr WriteBMP(const char* path, Color* pixels, uint32_t width, uint32_t height) {
	const uint32_t fileHeaderSize = 14;
	const uint32_t infoHeaderSize = 40;
	const uint32_t totalHeaderSize = fileHeaderSize + infoHeaderSize;
	const uint32_t pixelDataOffset = totalHeaderSize;
	const uint32_t planes = 1;
	const uint32_t bpp = 24;

	const uint32_t rowPixelSize = width * 3;
	const uint32_t rowPaddingSize = ((rowPixelSize + 3) & -4) - rowPixelSize;
	const uint32_t totalRowSize = rowPixelSize + rowPaddingSize;
	const uint32_t totalFileSize = totalHeaderSize + totalRowSize * height;

	FILE* outFile = TryOpenFile(path, "wb+");
	if (!outFile) return WRITE_BMP_FILE_ERR;

	if (!LuFileWriteChar(outFile, 'B')) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteChar(outFile, 'M')) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt32(outFile, totalFileSize)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt32(outFile, 0)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt32(outFile, pixelDataOffset)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);

	if (!LuFileWriteUInt32(outFile, infoHeaderSize)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt32(outFile, width)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt32(outFile, height)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt16(outFile, planes)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt16(outFile, bpp)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt32(outFile, 0)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt32(outFile, 0)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt32(outFile, 0)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt32(outFile, 0)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt32(outFile, 0)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);
	if (!LuFileWriteUInt32(outFile, 0)) WriteBMPCloseAndExit(WRITE_BMP_HEADER_ERR);

	for (int32_t y = height - 1; y >= 0; y--) {
		const Color* rowStart = pixels + (y * width);
		for (uint32_t x = 0; x < width; x++) {
			if (!LuFileWriteByte(outFile, rowStart[x].b)) WriteBMPCloseAndExit(WRITE_BMP_PIXELS_ERR);
			if (!LuFileWriteByte(outFile, rowStart[x].g)) WriteBMPCloseAndExit(WRITE_BMP_PIXELS_ERR);
			if (!LuFileWriteByte(outFile, rowStart[x].r)) WriteBMPCloseAndExit(WRITE_BMP_PIXELS_ERR);
		}

		for (uint32_t i = 0; i < rowPaddingSize; i++) {
			if (!LuFileWriteByte(outFile, 0)) WriteBMPCloseAndExit(WRITE_BMP_PADDING_ERR);
		}
	}

	WriteBMPCloseAndExit(WRITE_BMP_NO_ERR);
}

bool RasterTargetSaveToFile(const RasterTarget* screen, const char* path) {
	WriteBMPErr err = WriteBMP(path, screen->pixels, screen->width, screen->height);
	return (err == WRITE_BMP_NO_ERR);
}

void RasterTargetUpdateTexture(RasterTarget* screen) { UpdateTexture(screen->tex, screen->pixels); }

void RasterTargetRenderTexture(const RasterTarget* screen) { RasterTargetRenderTextureEx(screen, 1); }

void RasterTargetRenderTextureEx(const RasterTarget* screen, uint32_t scale) { DrawTextureEx(screen->tex, (Vector2){0}, 0.0f, scale, WHITE); }

void RasterTargetClearBackground(RasterTarget* screen, Color col) {
	const uint32_t pixelsCount = screen->width * screen->height;
	for (uint32_t i = 0; i < pixelsCount; i++) {
		screen->pixels[i] = col;
	}
}

static void RasterTargetDrawPixelFast(RasterTarget* screen, uint32_t x, uint32_t y, Color col) {
	screen->pixels[Index1D(x, y, screen->width)] = col;
}

void RasterTargetDrawPixel(RasterTarget* screen, uint32_t x, uint32_t y, Color col) {
	if (x >= screen->width || y >= screen->height) return;
	RasterTargetDrawPixelFast(screen, x, y, col);
}

static Vector2 Vector2Round(Vector2 v) {
	return (Vector2){
		.x = roundf(v.x),
		.y = roundf(v.y),
	};
}

void RasterTargetDrawTriangle(RasterTarget* screen, Vector2 a, Vector2 b, Vector2 c, Color col) {
	const Vector2 roundedA = Vector2Round(a);
	const Vector2 roundedB = Vector2Round(b);
	const Vector2 roundedC = Vector2Round(c);

	int32_t minX = screen->width;
	minX = Min(minX, roundedA.x);
	minX = Min(minX, roundedB.x);
	minX = Min(minX, roundedC.x);
	minX = Max(minX, 0);

	int32_t minY = screen->height;
	minY = Min(minY, roundedA.y);
	minY = Min(minY, roundedB.y);
	minY = Min(minY, roundedC.y);
	minY = Max(minY, 0);

	int32_t maxX = 0;
	maxX = Max(maxX, roundedA.x);
	maxX = Max(maxX, roundedB.x);
	maxX = Max(maxX, roundedC.x);
	maxX = Min(maxX, screen->width);

	int32_t maxY = 0;
	maxY = Max(maxY, roundedA.y);
	maxY = Max(maxY, roundedB.y);
	maxY = Max(maxY, roundedC.y);
	maxY = Min(maxY, screen->height);

	for (int32_t y = minY; y < maxY; y++) {
		for (int32_t x = minX; x < maxX; x++) {
			Vector2 pos = (Vector2){.x = x, .y = y};
			if (!CheckCollisionPointTriangle(pos, a, b, c)) continue;

			RasterTargetDrawPixelFast(screen, x, y, col);
		}
	}
}
