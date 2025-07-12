#include "main.h"

int main(void) {
	App app = {0};
	if (!AppInit(&app)) exit(EXIT_FAILURE);

	while (!WindowShouldClose()) {
		AppUpdate(&app);
		AppRender(&app);
	}
	AppClose(&app);

	exit(EXIT_SUCCESS);
}
