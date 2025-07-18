#ifndef APP_H
#define APP_H

#include "RasterTarget.h"

typedef struct App {
	RasterTarget* rasterTarget;

	RasterModel* cubeModel;
} App;

bool AppInit(App* app);
void AppUpdate(App* app);
void AppRender(const App* app);
void AppClose(App* app);

#endif	// APP_H
