#ifndef NK_GLES_GL32_H
#define NK_GLES_GL32_H

#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <assert.h>

#include <android/asset_manager.h>
#include <android/native_window.h>

/*#ifndef NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#endif
#ifndef NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_FONT_BAKING
#endif
#ifndef NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_DEFAULT_FONT
#endif
#ifndef NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#endif*/

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#include "nuklear.h"

enum nk_gles_init_state {
    NK_GLES32_DEFAULT = 0,
    NK_GLES32_INSTALL_CALLBACKS
};

#ifndef NK_GLES32_TEXT_MAX
#define NK_GLES32_TEXT_MAX 256
#endif

#define MAX_VERTEX_BUFFER  (512 * 1024)
#define MAX_ELEMENT_BUFFER (128 * 1024)

struct nk_gles_device {
    struct nk_buffer            cmds;
    struct nk_draw_null_texture tex_null;
    GLuint                      vbo, vao, ebo;
    GLuint                      prog;
    GLuint                      vert_shdr;
    GLuint                      frag_shdr;
    GLint                       attrib_pos;
    GLint                       attrib_uv;
    GLint                       attrib_col;
    GLint                       uniform_tex;
    GLint                       uniform_proj;
    GLuint                      font_tex;
};

struct nk_gles {
    struct ANativeWindow *win;
    int                   width, height;
    int                   display_width, display_height;
    struct nk_gles_device ogl;
    struct nk_context     ctx;
    struct nk_font_atlas  atlas;
    struct nk_vec2        fb_scale;
    unsigned int          text[NK_GLES32_TEXT_MAX];
    nk_char               key_events[NK_KEY_MAX];
    int                   text_len;
    struct nk_vec2        scroll;
    double                last_button_click;
    int                   is_double_click_down;
    struct nk_vec2        double_click_pos;
    float                 delta_time_seconds_last;
};

NK_API struct nk_context *nk_gles32_init(struct nk_gles *gles, struct ANativeWindow *win, struct AAssetManager *assMan);
NK_API void               nk_gles32_shutdown(struct nk_gles *gles);
NK_API void               nk_gles32_font_stash_begin(struct nk_gles *gles, struct nk_font_atlas **atlas);
NK_API void               nk_gles32_font_stash_end(struct nk_gles *gles);
NK_API void               nk_gles32_new_frame(struct nk_gles *gles);
NK_API void               nk_gles32_render(struct nk_gles *gles, enum nk_anti_aliasing, int max_vertex_buffer, int max_element_buffer);

NK_API void nk_gles32_device_destroy(struct nk_gles *gles);
NK_API void nk_gles32_device_create(struct nk_gles *gles, struct AAssetManager *assMan);

NK_API void nk_gles32_char_callback(struct ANativeWindow *win, unsigned int codepoint);
NK_API void nk_gles32_key_callback(struct ANativeWindow *win, int key, int scancode, int action, int mods);
NK_API void nk_gflw3_scroll_callback(struct ANativeWindow *win, double xoff, double yoff);
NK_API void nk_gles32_mouse_button_callback(struct ANativeWindow *win, int button, int action, int mods);

struct nk_gles_vertex {
    float   position[2];
    float   uv[2];
    nk_byte col[4];
};

// NOLINTNEXTLINE
#define NK_SHADER_VERSION "#version 300 es\n"
#endif
