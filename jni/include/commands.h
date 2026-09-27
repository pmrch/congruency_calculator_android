#ifndef COMMANDS_H
#define COMMANDS_H

#include "android_native_app_glue.h"
#include "render.h"

void cmd_content_rect_changed(ANativeActivity *activity, const ARect *r, ARect *g_content_rect);

void cmd_init_window(struct android_app *app, int32_t *rnd_init);
void cmd_activity_destroyed(int32_t *g_running);
void cmd_term_window(Renderer *rnd);

#endif
