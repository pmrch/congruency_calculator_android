#include "commands.h"
#include "render.h"
#include "utils.h"

void cmd_init_window(struct android_app *app, int32_t *rnd_init) {
    UserData *data = (UserData *)app->userData;
    LOGI("%s", "Window initialized");

    if (app->window != NULL) {
        int32_t width  = ANativeWindow_getWidth(app->window);
        int32_t height = ANativeWindow_getHeight(app->window);

        LOGI("Window size: %d x %d", width, height);
        *rnd_init = rendererInit(app->window, &data->rnd);

        if (*rnd_init) {
            LOGI("%s", "Renderer initialized successfully on window creation.");
            data->nkCtx = nk_gles32_init(&data->gles, app->window, app->activity->assetManager);
            if (data->nkCtx != NULL) { LOGI("%s", "Successfully initialized Nuklear"); }
        }
    }
}

void cmd_content_rect_changed(ANativeActivity *activity, const ARect *r, ARect *g_content_rect) {
    if (activity != NULL && r != NULL) {
        LOGI("Detected content rectangle changes: (%d, %d, %d, %d)", r->left, r->top, r->right, r->bottom);

        g_content_rect->bottom = r->bottom;
        g_content_rect->top    = r->top;
        g_content_rect->right  = r->right;
        g_content_rect->left   = r->left;
    }
}

void cmd_activity_destroyed(int32_t *g_running) {
    LOGI("%s", "Activity destroyed");
    *g_running = 0;
}

void cmd_term_window(Renderer *rnd) {
    LOGI("%s", "Window terminated");
    destroyRenderer(rnd);
    LOGI("%s", "Renderer destroyed");
}
