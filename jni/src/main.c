#include <stdbool.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl32.h>

#include "android_native_app_glue.h"
#include <android/asset_manager.h>
#include <android/input.h>
#include <android/native_window.h>
#include <android/window.h>

#include "commands.h"
#include "render.h"
#include "ui.h"
#include "utils.h"

// Convenient type definition
typedef struct android_poll_source poll_src;

// Active main loop
static UserData userData       = {.gles = {0}, .nkCtx = NULL, .rnd = {0}, .g_running = 1, .rnd_init = 0};
static ARect    g_content_rect = {0, 0, 0, 0};

static void getMaxDims(WindowDimensions *dims) {
    dims->min_width  = g_content_rect.left;
    dims->max_width  = g_content_rect.right;
    dims->min_height = g_content_rect.top;
    dims->max_height = g_content_rect.bottom;
}

static void handle_cmd(struct android_app *app, int32_t cmd) {
    UserData *localUserData = (UserData *)app->userData;

    if (cmd == APP_CMD_CONTENT_RECT_CHANGED) { cmd_content_rect_changed(app->activity, &app->contentRect, &g_content_rect); }
    if (cmd == APP_CMD_INIT_WINDOW) { cmd_init_window(app, &localUserData->rnd_init); }
    if (cmd == APP_CMD_DESTROY) { cmd_activity_destroyed(&localUserData->g_running); }
    if (cmd == APP_CMD_RESUME) { LOGI("%s", "Activity resumed"); }
    if (cmd == APP_CMD_PAUSE) { LOGI("%s", "Activity paused"); }

    if (cmd == APP_CMD_TERM_WINDOW) {
        cmd_term_window(&localUserData->rnd);
        localUserData->g_running = false;
    }
}

void android_main(struct android_app *state) {
    WindowDimensions dims         = {0};
    int32_t          needs_redraw = false;

    state->userData     = (void *)(&userData);
    state->onAppCmd     = handle_cmd;
    state->onInputEvent = handleInput;

    ANativeActivity_setWindowFlags(state->activity, AWINDOW_FLAG_LAYOUT_IN_SCREEN | AWINDOW_FLAG_LAYOUT_INSET_DECOR, 0);

    while (userData.g_running) {
        int       events, ident;
        poll_src *source;

        int timeout = (!userData.rnd_init || !needs_redraw) ? -1 : 0;

        /* Start Nuklear input collection */
        nk_input_begin(userData.nkCtx);

        // Poll once per iteration
        ident = ALooper_pollOnce(timeout, NULL, &events, (void **)&source);

        if (ident >= 0) {
            if (source != NULL) { source->process(state, source); }
            needs_redraw = true;
        }

        /* Finish Nuklear input collection */
        nk_input_end(userData.nkCtx);

        if (userData.rnd_init && needs_redraw && state->window != NULL && userData.nkCtx != NULL) {
            nk_gles32_new_frame(&userData.gles);

            dims.width  = ANativeWindow_getWidth(state->window);
            dims.height = ANativeWindow_getHeight(state->window);
            getMaxDims(&dims);

            LOGI("WINDOW: %d x %d", dims.width, dims.height);
            LOGI("NK WINDOW: %.1f x %.1f", (float)dims.width, (float)dims.height);
            LOGI("NK mouse: %.1f %.1f", userData.nkCtx->input.mouse.pos.x, userData.nkCtx->input.mouse.pos.y);

            drawUi(userData.nkCtx, &dims);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            nk_gles32_render(&userData.gles, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
            eglSwapBuffers(userData.rnd.display, userData.rnd.surface);
            needs_redraw = false;
        }
    }

    if (!userData.g_running) { LOGE("%s", "Not running anymore!"); }
}
