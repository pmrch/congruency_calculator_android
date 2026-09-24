#ifndef RENDER_H
#define RENDER_H

#include <EGL/egl.h>
#include <android/native_window.h>

typedef struct {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
} Renderer;

int32_t rendererInit(ANativeWindow *window, Renderer *rnd);
void    destroyRenderer(Renderer *rnd);

#endif
