#include "render.h"
#include "utils.h"

// clang-format off
// Initializes the display surface with at least 8 bits per RGB colors
// Surface type is set to EGL Window and render type is OpenGL ES3
static const EGLint attribs[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT, 

    EGL_BLUE_SIZE, 8, 
    EGL_GREEN_SIZE, 8, 
    EGL_RED_SIZE, 8,
    EGL_ALPHA_SIZE, 8, 
    
    EGL_NONE
};
// clang-format on

static const EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};

int32_t rendererInit(ANativeWindow *window, Renderer *rnd) {
    EGLConfig config;
    EGLint    num_configs;

    // Bail out early if prerequisites are not met
    if (window == NULL || rnd == NULL) {
        LOGE("Failed to initialize renderer: Window or renderer object was NULL!");
        return 0;
    }

    // Acquire current display
    rnd->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (rnd->display == EGL_NO_DISPLAY) {
        LOGE("Failed to initialize EGL display!");
        return 0;
    }

    // Initialize the EGL bridge
    if (!eglInitialize(rnd->display, NULL, NULL)) {
        LOGE("Failed to initialize the EGL bride!");
        return 0;
    }

    // Configure EGL settings
    if (!eglChooseConfig(rnd->display, attribs, &config, 1, &num_configs)) {
        LOGE("Failed to choose configuration!");
        destroyRenderer(rnd);
        return 0;
    }

    // Set up EGL API with OpenGL ES
    eglBindAPI(EGL_OPENGL_ES_API);

    // Create EGL context
    rnd->context = eglCreateContext(rnd->display, config, EGL_NO_CONTEXT, context_attributes);
    if (rnd->context == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL conxtext!");
        destroyRenderer(rnd);
        return 0;
    }

    // Create the actual display surface
    rnd->surface = eglCreateWindowSurface(rnd->display, config, window, NULL);
    if (rnd->surface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL window surface!");
        destroyRenderer(rnd);
        return 0;
    }

    // Finish EGL configuration by applying all the settings
    if (!eglMakeCurrent(rnd->display, rnd->surface, rnd->surface, rnd->context)) {
        LOGE("Failed to finalize EGL settings!");
        destroyRenderer(rnd);
        return 0;
    }

    return 1;
}

void destroyRenderer(Renderer *rnd) {
    eglMakeCurrent(rnd->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(rnd->display, rnd->surface);
    eglDestroyContext(rnd->display, rnd->context);
    eglTerminate(rnd->display);
}
