#include <stdbool.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl32.h>

#include "android_native_app_glue.h"
#include <android/asset_manager.h>
#include <android/native_window.h>

#include "commands.h"
#include "render.h"
#include "utils.h"

// Convenient type definition
typedef struct android_poll_source poll_src;

// Active main loop
static UserData userData = {.gles = {0}, .nkCtx = NULL, .rnd = {0}, .g_running = 1, .rnd_init = 0};

static void handle_cmd(struct android_app *app, int32_t cmd) {
    UserData *localUserData = (UserData *)app->userData;

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
    int32_t needs_redraw = false;

    state->userData = (void *)(&userData);
    state->onAppCmd = handle_cmd;

    while (userData.g_running) {
        int       events;
        poll_src *source;

        int timeout = (!userData.rnd_init || !needs_redraw) ? -1 : 0;

        // Poll once per iteration
        int ident = ALooper_pollOnce(timeout, NULL, &events, (void **)&source);

        if (ident >= 0) {
            if (source != NULL) { source->process(state, source); }
            needs_redraw = true;
        }

        if (userData.rnd_init && needs_redraw && state->window != NULL && userData.nkCtx != NULL) {
            nk_gles32_new_frame(&userData.gles);

            nk_input_begin(userData.nkCtx);
            nk_input_end(userData.nkCtx);

            if (nk_begin(userData.nkCtx, "Test", nk_rect(50, 150, 400, 200), NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {
                nk_layout_row_dynamic(userData.nkCtx, 30, 1);
                nk_label(userData.nkCtx, "Hello from Nuklear!", NK_TEXT_LEFT);
                nk_end(userData.nkCtx);
            }

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            nk_gles32_render(&userData.gles, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
            eglSwapBuffers(userData.rnd.display, userData.rnd.surface);
            needs_redraw = false;
        }
    }
}
