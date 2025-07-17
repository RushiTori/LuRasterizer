#ifndef RASTER_MODEL_H
#define RASTER_MODEL_H

#include "RasterCommon.h"

typedef struct RasterModelFace {
	uint16_t* vertexIndeces;
	uint16_t* texCoordIndices;
	uint16_t* normalIndices;
	size_t indicesSize;
} RasterModelFace;

typedef struct RasterModel {
	Vector3* vertices;
	size_t verticesSize;

	Vector2* texCoords;
	size_t texCoordsSize;

	Vector3* normals;
	size_t normalsSize;

	RasterModelFace* faces;
	size_t facesSize;
} RasterModel;

RasterModel* LoadRasterModelFromFile(const char* path);
void RasterModelFree(RasterModel* model);

#endif	// RASTER_MODEL_H
