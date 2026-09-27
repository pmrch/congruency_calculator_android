#ifndef RENDER_H
#define RENDER_H

#include <EGL/egl.h>

#include "nuklear_android_gles32.h"
#include <android/native_window.h>

typedef struct nk_context NkContext;
typedef struct nk_gles    NkGles;

typedef struct {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
} Renderer;

typedef struct {
    NkGles     gles;
    Renderer   rnd;
    NkContext *nkCtx;
    int32_t    g_running;
    int32_t    rnd_init;
} UserData;

int32_t rendererInit(ANativeWindow *window, Renderer *rnd);
void    destroyRenderer(Renderer *rnd);

#endif
