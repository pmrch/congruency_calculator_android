#include "commands.h"
#include "render.h"
#include "utils.h"

void cmd_init_window(struct android_app *app, Renderer *rnd, int32_t *renderer_initialized) {
    LOGI("Window initialized");

    if (app->window != NULL) {
        int32_t width  = ANativeWindow_getWidth(app->window);
        int32_t height = ANativeWindow_getHeight(app->window);

        LOGI("Window size: %d x %d", width, height);
        *renderer_initialized = rendererInit(app->window, rnd);

        if (*renderer_initialized) { LOGI("Renderer initialized successfully on window creation."); }
    }
}

void cmd_activity_destroyed(int32_t *g_running) {
    LOGI("Activity destroyed");
    *g_running = 0;
}

void cmd_term_window(Renderer *rnd) {
    LOGI("Window terminated");
    destroyRenderer(rnd);
    LOGI("Renderer destroyed");
}
