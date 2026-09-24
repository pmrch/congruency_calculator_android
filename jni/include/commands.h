#ifndef COMMANDS_H
#define COMMANDS_H

#include "android_native_app_glue.h"
#include "render.h"

void cmd_init_window(struct android_app *app, Renderer *rnd, int32_t *renderer_initialized);
void cmd_activity_destroyed(int32_t *g_running);
void cmd_term_window(Renderer *rnd);

#endif
