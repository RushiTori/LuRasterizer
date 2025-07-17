#include "RasterModel.h"

#include <LuLib/LuArray.h>
#include <regex.h>

DeclareArrayType(Vector2, Vec2Array);
DeclareArrayMethods(Vector2, Vec2Array);
DefineArrayMethods(Vector2, Vec2Array);

DeclareArrayType(Vector3, Vec3Array);
DeclareArrayMethods(Vector3, Vec3Array);
DefineArrayMethods(Vector3, Vec3Array);

DeclareArrayType(uint16_t, IndexArray);
DeclareArrayMethods(uint16_t, IndexArray);
DefineArrayMethods(uint16_t, IndexArray);

DeclareArrayType(RasterModelFace, FaceArray);
DeclareArrayMethods(RasterModelFace, FaceArray);
DefineArrayMethods(RasterModelFace, FaceArray);

#define DEFAULT_ARRAY_CAPACITY 8

#define OBJ_VERTEX_PREFIX "v "
#define OBJ_VERTEX_PREFIX_LEN strlen(OBJ_VERTEX_PREFIX)

#define OBJ_TEXCOORDS_PREFIX "vt "
#define OBJ_TEXCOORDS_PREFIX_LEN strlen(OBJ_TEXCOORDS_PREFIX)

#define OBJ_NORMAL_PREFIX "vn "
#define OBJ_NORMAL_PREFIX_LEN strlen(OBJ_NORMAL_PREFIX)

#define OBJ_FACE_PREFIX "f "
#define OBJ_FACE_PREFIX_LEN strlen(OBJ_FACE_PREFIX)

#define FLOAT_PATTERN "(\\-?[0-9]+\\.[0-9]+)"

#define VEC2REG_PATTERN (FLOAT_PATTERN " " FLOAT_PATTERN)
#define VEC3REG_PATTERN (FLOAT_PATTERN " " FLOAT_PATTERN " " FLOAT_PATTERN)

#define REG_ERROR_BUFFER_LEN 2048

typedef struct LineInfo {
	char* data;
	int64_t size;
	size_t capacity;

	const char* filePath;
	size_t lineNumber;
} LineInfo;

typedef struct IndicesLimit {
	uint16_t vertices;
	uint16_t texCoords;
	uint16_t normals;
} IndicesLimit;

static Vector3 ParseVertexLine(const LineInfo* line, regex_t* vec3reg) {
	char* data = line->data + OBJ_VERTEX_PREFIX_LEN;

	regmatch_t matches[4] = {0};
	int32_t execErr = regexec(vec3reg, data, 4, matches, 0);
	if (execErr == REG_NOMATCH) {
		LogMessage("Ill-formed vertex info at %s:%zu (defaulting to {0, 0, 0})\n", line->filePath, line->lineNumber);
		return (Vector3){0};
	}

	if (execErr) {
		char regErrBuffer[REG_ERROR_BUFFER_LEN] = {0};

		size_t bufEnd = regerror(execErr, vec3reg, regErrBuffer, REG_ERROR_BUFFER_LEN);
		regErrBuffer[bufEnd - 1] = '\0';
		LogMessage("Vertex regex error at %s:%zu (defaulting to {0, 0, 0})\n\t%s", line->filePath, line->lineNumber, regErrBuffer);
		return (Vector3){0};
	}

	return (Vector3){
		.x = strtof(data + matches[1].rm_so, NULL),
		.y = strtof(data + matches[2].rm_so, NULL),
		.z = strtof(data + matches[3].rm_so, NULL),
	};
}

static Vector2 ParseTexCoordsLine(const LineInfo* line, regex_t* vec2reg) {
	char* data = line->data + OBJ_TEXCOORDS_PREFIX_LEN;

	regmatch_t matches[4] = {0};
	int32_t execErr = regexec(vec2reg, data, 4, matches, 0);
	if (execErr == REG_NOMATCH) {
		LogMessage("Ill-formed texture coordinates info at %s:%zu (defaulting to {0, 0})\n", line->filePath, line->lineNumber);
		return (Vector2){0};
	}

	if (execErr) {
		char regErrBuffer[REG_ERROR_BUFFER_LEN] = {0};

		size_t bufEnd = regerror(execErr, vec2reg, regErrBuffer, REG_ERROR_BUFFER_LEN);
		regErrBuffer[bufEnd - 1] = '\0';
		LogMessage("Texture coordinates regex error at %s:%zu (defaulting to {0, 0})\n\t%s", line->filePath, line->lineNumber, regErrBuffer);
		return (Vector2){
			.x = 0,
			.y = 0,
		};
	}

	return (Vector2){
		.x = strtof(data + matches[1].rm_so, NULL),
		.y = strtof(data + matches[2].rm_so, NULL),
	};
}

static Vector3 ParseNormalLine(const LineInfo* line, regex_t* vec3reg) {
	char* data = line->data + OBJ_NORMAL_PREFIX_LEN;

	regmatch_t matches[4] = {0};
	int32_t execErr = regexec(vec3reg, data, 4, matches, 0);
	if (execErr == REG_NOMATCH) {
		LogMessage("Ill-formed normal info at %s:%zu (defaulting to {0, 0, 0})\n", line->filePath, line->lineNumber);
		return (Vector3){0};
	}

	if (execErr) {
		char regErrBuffer[REG_ERROR_BUFFER_LEN] = {0};

		size_t bufEnd = regerror(execErr, vec3reg, regErrBuffer, REG_ERROR_BUFFER_LEN);
		regErrBuffer[bufEnd - 1] = '\0';
		LogMessage("Normal regex error at %s:%zu (defaulting to {0, -1, 0})\n\t%s", line->filePath, line->lineNumber, regErrBuffer);
		return (Vector3){
			.x = 0,
			.y = -1,
			.z = 0,
		};
	}

	return Vector3Normalize((Vector3){
		.x = strtof(data + matches[1].rm_so, NULL),
		.y = strtof(data + matches[2].rm_so, NULL),
		.z = strtof(data + matches[3].rm_so, NULL),
	});
}

#define ParseFaceLineExitFail()                             \
	{                                                       \
		if (vertexArray) IndexArrayFree(vertexArray);       \
		if (texCoordsArray) IndexArrayFree(texCoordsArray); \
		if (normalArray) IndexArrayFree(normalArray);       \
		return (RasterModelFace){0};                        \
	}

static RasterModelFace ParseFaceLine(const LineInfo* line, const IndicesLimit* maxIndices) {
	char* ptr = line->data + OBJ_FACE_PREFIX_LEN;
	char* next = ptr;

	IndexArray* vertexArray = IndexArrayCreate(DEFAULT_ARRAY_CAPACITY);
	IndexArray* texCoordsArray = IndexArrayCreate(DEFAULT_ARRAY_CAPACITY);
	IndexArray* normalArray = IndexArrayCreate(DEFAULT_ARRAY_CAPACITY);

	if (!vertexArray || !texCoordsArray || !normalArray) ParseFaceLineExitFail();

	while (*next) {
#define CheckIndexError(msg)     \
	{                            \
		LogString(msg);          \
		ParseFaceLineExitFail(); \
	}

#define CheckIndex(idx, maxIdx)                                                                          \
	{                                                                                                    \
		int64_t chkIdxTemp1_ = strtoll(ptr, &next, 10);                                                  \
		if (next == ptr) CheckIndexError("TODO: Add error message for invalid model \"face\" line\n");   \
                                                                                                         \
		if (chkIdxTemp1_ < 0) chkIdxTemp1_ = maxIdx + chkIdxTemp1_ + 1;                                  \
		idx = chkIdxTemp1_ - 1;                                                                          \
                                                                                                         \
		if (idx >= maxIdx) CheckIndexError("TODO: Add error message for invalid model \"face\" line\n"); \
	}

		uint16_t currVertexIdx = 0;
		uint16_t currTexCoordsIdx = 0;
		uint16_t currNormalIdx = 0;

		CheckIndex(currVertexIdx, maxIndices->vertices);
		next++;
		ptr = next;

		CheckIndex(currTexCoordsIdx, maxIndices->texCoords);
		next++;
		ptr = next;

		CheckIndex(currNormalIdx, maxIndices->normals);
		next++;
		ptr = next;

		bool failedPush = false;
		if (!IndexArrayPush(vertexArray, currVertexIdx)) failedPush = true;
		if (!IndexArrayPush(texCoordsArray, currTexCoordsIdx)) failedPush = true;
		if (!IndexArrayPush(normalArray, currNormalIdx)) failedPush = true;
		if (failedPush) ParseFaceLineExitFail();
	}

	bool failedShrink = false;
	if (!IndexArrayShrinkToFit(vertexArray)) failedShrink = true;
	if (!IndexArrayShrinkToFit(texCoordsArray)) failedShrink = true;
	if (!IndexArrayShrinkToFit(normalArray)) failedShrink = true;
	if (failedShrink) ParseFaceLineExitFail();

	RasterModelFace face = (RasterModelFace){
		.vertexIndeces = vertexArray->data,
		.texCoordIndices = texCoordsArray->data,
		.normalIndices = normalArray->data,
		.indicesSize = vertexArray->size,
	};

	Free(vertexArray);
	Free(texCoordsArray);
	Free(normalArray);

	return face;
}

#define LoadRasterModelFromFileExitFail()                    \
	{                                                        \
		regfree(&vec3reg);                                   \
		regfree(&vec2reg);                                   \
		if (parsedVertices) Vec3ArrayFree(parsedVertices);   \
		if (parsedTexCoords) Vec2ArrayFree(parsedTexCoords); \
		if (parsedNormals) Vec3ArrayFree(parsedNormals);     \
		if (parsedFaces) FaceArrayFree(parsedFaces);         \
		CloseFile(objFile);                                  \
		FreeAndReturn(model, NULL);                          \
	}

RasterModel* LoadRasterModelFromFile(const char* path) {
	RasterModel* model = NULL;
	if (!Malloc(model, sizeof(RasterModel))) return NULL;

	FILE* objFile = TryOpenFile(path, "r+");
	if (!objFile) FreeAndReturn(model, NULL);

	regex_t vec3reg;
	regex_t vec2reg;
	{
		char regErrBuffer[REG_ERROR_BUFFER_LEN] = {0};
		int32_t regErr1;
		int32_t regErr2;
		bool failedRegcomp = false;

		if ((regErr1 = regcomp(&vec3reg, VEC3REG_PATTERN, REG_EXTENDED))) {
			failedRegcomp = true;
			size_t bufEnd = regerror(regErr1, &vec3reg, regErrBuffer, REG_ERROR_BUFFER_LEN);
			regErrBuffer[bufEnd - 1] = '\0';
			LogString(regErrBuffer);
		}

		if ((regErr2 = regcomp(&vec2reg, VEC2REG_PATTERN, REG_EXTENDED))) {
			failedRegcomp = true;
			size_t bufEnd = regerror(regErr2, &vec2reg, regErrBuffer, REG_ERROR_BUFFER_LEN);
			regErrBuffer[bufEnd - 1] = '\0';
			LogString(regErrBuffer);
		}

		if (failedRegcomp) {
			if (!regErr1) regfree(&vec3reg);
			if (!regErr2) regfree(&vec2reg);
			CloseFile(objFile);
			FreeAndReturn(model, NULL);
		}
	}

	Vec3Array* parsedVertices = Vec3ArrayCreate(DEFAULT_ARRAY_CAPACITY);
	Vec2Array* parsedTexCoords = Vec2ArrayCreate(DEFAULT_ARRAY_CAPACITY);
	Vec3Array* parsedNormals = Vec3ArrayCreate(DEFAULT_ARRAY_CAPACITY);
	FaceArray* parsedFaces = FaceArrayCreate(DEFAULT_ARRAY_CAPACITY);

	if (!parsedVertices || !parsedTexCoords || !parsedNormals || !parsedFaces) {
		LoadRasterModelFromFileExitFail();
	}

	LineInfo line = {0};
	line.filePath = path;
	IndicesLimit maxIndices = {0};

	while ((line.size = LuFileGetLine(&line.data, &line.capacity, objFile)) != LU_FILE_ERROR) {
		line.lineNumber++;
#define CheckForPrefix(prefix) if (strncmp(line.data, prefix, prefix##_LEN) == 0)

		CheckForPrefix(OBJ_VERTEX_PREFIX) {
			if (!Vec3ArrayPush(parsedVertices, ParseVertexLine(&line, &vec3reg))) {
				Free(line.data);
				LoadRasterModelFromFileExitFail();
			}
			maxIndices.vertices++;
		}

		CheckForPrefix(OBJ_TEXCOORDS_PREFIX) {
			if (!Vec2ArrayPush(parsedTexCoords, ParseTexCoordsLine(&line, &vec2reg))) {
				Free(line.data);
				LoadRasterModelFromFileExitFail();
			}
			maxIndices.texCoords++;
		}

		CheckForPrefix(OBJ_NORMAL_PREFIX) {
			if (!Vec3ArrayPush(parsedNormals, ParseNormalLine(&line, &vec3reg))) {
				Free(line.data);
				LoadRasterModelFromFileExitFail();
			}
			maxIndices.normals++;
		}

		CheckForPrefix(OBJ_FACE_PREFIX) {
			if (!FaceArrayPush(parsedFaces, ParseFaceLine(&line, &maxIndices))) {
				Free(line.data);
				LoadRasterModelFromFileExitFail();
			}
		}
	}
	Free(line.data);

	bool failedShrink = false;
	if (!Vec3ArrayShrinkToFit(parsedVertices)) failedShrink = true;
	if (!Vec2ArrayShrinkToFit(parsedTexCoords)) failedShrink = true;
	if (!Vec3ArrayShrinkToFit(parsedNormals)) failedShrink = true;
	if (!FaceArrayShrinkToFit(parsedFaces)) failedShrink = true;
	if (failedShrink) LoadRasterModelFromFileExitFail();

	*model = (RasterModel){
		.vertices = parsedVertices->data,
		.verticesSize = parsedVertices->size,

		.texCoords = parsedTexCoords->data,
		.texCoordsSize = parsedTexCoords->size,

		.normals = parsedNormals->data,
		.normalsSize = parsedNormals->size,

		.faces = parsedFaces->data,
		.facesSize = parsedFaces->size,
	};

	regfree(&vec3reg);
	regfree(&vec2reg);
	Free(parsedVertices);
	Free(parsedTexCoords);
	Free(parsedNormals);
	Free(parsedFaces);
	CloseFile(objFile);

	return model;
}

void RasterModelFree(RasterModel* model) {
	Free(model->vertices);
	Free(model->texCoords);
	Free(model->normals);

	Free(model->faces->vertexIndeces);
	Free(model->faces->texCoordIndices);
	Free(model->faces->normalIndices);
	Free(model->faces);

	Free(model);
}
