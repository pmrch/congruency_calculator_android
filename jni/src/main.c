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
static int32_t  g_running            = 1;
static int32_t  renderer_initialized = false;
static Renderer rnd                  = {0};

static void handle_cmd(struct android_app *app, int32_t cmd) {
    if (cmd == APP_CMD_INIT_WINDOW) { cmd_init_window(app, &rnd, &renderer_initialized); }
    if (cmd == APP_CMD_DESTROY) { cmd_activity_destroyed(&g_running); }
    if (cmd == APP_CMD_RESUME) { LOGI("Activity resumed"); }
    if (cmd == APP_CMD_PAUSE) { LOGI("Activity paused"); }

    if (cmd == APP_CMD_TERM_WINDOW) {
        cmd_term_window(&rnd);
        renderer_initialized = false;
    }
}

void android_main(struct android_app *state) {
    int32_t needs_redraw = false;

    state->onAppCmd = handle_cmd;
    g_running       = 1;

    while (g_running) {
        int       events;
        poll_src *source;

        int timeout = (!renderer_initialized || !needs_redraw) ? -1 : 0;

        // Poll once per iteration
        int ident = ALooper_pollOnce(timeout, NULL, &events, (void **)&source);

        if (ident >= 0) {
            if (source != NULL) { source->process(state, source); }
            needs_redraw = true;
        }

        if (renderer_initialized && needs_redraw) {

            // Clear the flag so we go back to sleep until the next interaction
            needs_redraw = false;
        }
    }
}
