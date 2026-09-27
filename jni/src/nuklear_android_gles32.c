#define NK_IMPLEMENTATION
#include "nuklear_android_gles32.h"
#include <android/asset_manager.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "utils.h"

// Avoid using string literals as shaders by loading from assets
static char *loadShaderAsset(struct AAssetManager *assMan, const char *const filename, size_t offset) {
    AAsset *ass    = NULL;
    char   *buffer = NULL;
    int64_t length = 0, bytesRead = 0;

    if (assMan == NULL || filename == NULL) {
        LOGE("Failed to load shader asset %s", filename);
        return NULL;
    }

    if ((ass = AAssetManager_open(assMan, filename, AASSET_MODE_BUFFER)) == NULL) {
        LOGE("Failed to open asset: %s", filename);
        return NULL;
    }

    length = AAsset_getLength(ass);
    if ((buffer = (char *)malloc(offset + (size_t)length + 1)) == NULL) {
        LOGE("Failed to allocate memory for buffer for asset %s", filename);
        AAsset_close(ass);
        return NULL;
    }

    memcpy(buffer, NK_SHADER_VERSION, offset);
    if ((bytesRead = AAsset_read(ass, (buffer + offset), (size_t)length)) < 0) {
        LOGE("Failed to read the asset %s due to error: %s", filename, strerror(errno));
        FREE_ALL(TO_DFREE(buffer), TO_FREE(ass, AAsset_close));
        return NULL;
    }

    buffer[offset + (size_t)bytesRead] = '\0';
    LOGV("Read the following shader: %s", buffer);
    AAsset_close(ass);

    return buffer;
}

// Replace assertion checks with error handling
static int32_t check_shader_status(GLint status, GLuint shader) {
    if (status != GL_TRUE) {
        GLchar log[512] = {0};
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);

        LOGE("Vertex shader compilation failed: %s", log);
        return -1;
    }

    return 0;
}

// Helper function to get monotonic time in seconds on Android
static float android_get_time(void) {
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    return (float)res.tv_sec + (float)res.tv_nsec * 1e-9f;
}

// Setup font default
static void setup_default_font(struct nk_gles *gles, struct nk_font **font) {
    int         w, h;
    GLuint      font_tex_id;
    const void *image = NULL;

    struct nk_font_atlas atlas = {0};
    nk_font_atlas_init_default(&atlas);
    nk_font_atlas_begin(&atlas);

    *font = nk_font_atlas_add_default(&atlas, 18.0f, NULL);
    if (*font == NULL) {
        LOGE("%s", "Failed to setup default font!");
        nk_font_atlas_clear(&atlas);
        return;
    }

    image = nk_font_atlas_bake(&atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    if (image == NULL) {
        LOGE("%s", "Failed to setup default font: font atlas baking failed!");
        nk_font_atlas_clear(&atlas);
        return;
    }

    LOGI("-> Font baked successfully, w=%d, h=%d", w, h);

    // Generate an OpenGL texture ID for the font
    glGenTextures(1, &font_tex_id);
    LOGI("-> Gen textures passed, id: %u", font_tex_id);

    glBindTexture(GL_TEXTURE_2D, font_tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Set standard texture filtering parameters
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("%s", "-> Texture uploaded to GPU successfully");

    // Tell Nuklear the texture ID so it knows how to draw text
    nk_font_atlas_end(&atlas, nk_handle_id((int)font_tex_id), &gles->ogl.tex_null);

    // Save the texture ID so the renderer can bind it later
    gles->ogl.font_tex = font_tex_id;

    /*// Clean up atlas helper
    nk_font_atlas_clear(&atlas);*/
    LOGI("%s", "Finished default font setup");
}

NK_API void nk_gles32_device_create(struct nk_gles *gles, struct AAssetManager *assMan) {
    GLint  status;
    size_t offset = strlen(NK_SHADER_VERSION);

    GLchar *vertex_shader_buf   = loadShaderAsset(assMan, "vertex.glsl", offset);
    GLchar *fragment_shader_buf = loadShaderAsset(assMan, "fragment.glsl", offset);

    const GLchar *vertex_shader   = vertex_shader_buf;
    const GLchar *fragment_shader = fragment_shader_buf;

    struct nk_gles_device *dev = &gles->ogl;
    if (vertex_shader_buf == NULL) { return; }

    if (fragment_shader_buf == NULL) {
        free(vertex_shader_buf);
        return;
    }

    nk_buffer_init_default(&dev->cmds);

    dev->prog      = glCreateProgram();
    dev->vert_shdr = glCreateShader(GL_VERTEX_SHADER);
    dev->frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);

    // Load shader source onto the GPU
    glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
    glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);

    // Free temporary buffers
    FREE_ALL(TO_DFREE(vertex_shader_buf), TO_DFREE(fragment_shader_buf));

    // Compile shaders on the GPU
    glCompileShader(dev->vert_shdr);
    glCompileShader(dev->frag_shdr);

    // Verify compilation succeeded
    glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
    if (check_shader_status(status, dev->vert_shdr) != 0) { return; }
    glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
    if (check_shader_status(status, dev->frag_shdr) != 0) { return; }

    // Attach compiled shaders to the program and link
    glAttachShader(dev->prog, dev->vert_shdr);
    glAttachShader(dev->prog, dev->frag_shdr);
    glLinkProgram(dev->prog);

    // Verify linking succeeded
    glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);

    // Retrieve uniform and attribute locations from the linked program
    dev->uniform_tex  = glGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
    dev->attrib_pos   = glGetAttribLocation(dev->prog, "Position");
    dev->attrib_uv    = glGetAttribLocation(dev->prog, "TexCoord");
    dev->attrib_col   = glGetAttribLocation(dev->prog, "Color");

    {
        // Cache vertex stride and attribute offsets for layout description
        GLsizei vs = sizeof(struct nk_gles_vertex);
        size_t  vp = offsetof(struct nk_gles_vertex, position);
        size_t  vt = offsetof(struct nk_gles_vertex, uv);
        size_t  vc = offsetof(struct nk_gles_vertex, col);

        // Generate GPU buffer and vertex array objects
        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
        glGenVertexArrays(1, &dev->vao);

        // Bind VAO and buffers to record the layout state
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        // Enable vertex attribute slots
        glEnableVertexAttribArray((GLuint)dev->attrib_pos);
        glEnableVertexAttribArray((GLuint)dev->attrib_uv);
        glEnableVertexAttribArray((GLuint)dev->attrib_col);

        // Describe vertex layout: position (vec2), uv (vec2), color (rgba ubyte)
        glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void *)vp);
        glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void *)vt);
        glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void *)vc);
    }

    // Unbind everything to avoid accidental state pollution
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

NK_INTERN void nk_gles32_device_upload_atlas(struct nk_gles *gles, const void *image, int width, int height) {
    struct nk_gles_device *dev = &gles->ogl;
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
}

NK_API void nk_gles32_device_destroy(struct nk_gles *gles) {
    struct nk_gles_device *dev = &gles->ogl;
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteTextures(1, &dev->font_tex);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    nk_buffer_free(&dev->cmds);
}

// max_vertex_buffer must be the exact size of buffer in bytes
NK_API void nk_gles32_render(struct nk_gles *gles, enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer) {
    struct nk_gles_device *dev = &gles->ogl;
    struct nk_buffer       vbuf, ebuf;

    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, -2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
    };

    ortho[0][0] /= (GLfloat)gles->width;
    ortho[1][1] /= (GLfloat)gles->height;

    /* setup global state */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    glViewport(0, 0, (GLsizei)gles->display_width, (GLsizei)gles->display_height);

    {
        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        void                         *vertices, *elements;
        nk_size                       offset = 0;

        /* allocate vertex and element buffer */
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW);

        /* load draw vertices & elements directly into vertex + element buffer */
        vertices = glMapBufferRange(GL_ARRAY_BUFFER, 0, max_vertex_buffer, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        elements = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, max_element_buffer, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        {
            /* fill convert configuration */
            struct nk_convert_config                          config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_gles_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_gles_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_gles_vertex, col)},
                {NK_VERTEX_LAYOUT_END}};

            memset(&config, 0, sizeof(config));
            config.vertex_layout        = vertex_layout;
            config.vertex_size          = sizeof(struct nk_gles_vertex);
            config.vertex_alignment     = NK_ALIGNOF(struct nk_gles_vertex);
            config.tex_null             = dev->tex_null;
            config.circle_segment_count = 22;
            config.curve_segment_count  = 22;
            config.arc_segment_count    = 22;
            config.global_alpha         = 1.0f;
            config.shape_AA             = AA;
            config.line_AA              = AA;

            /* setup buffers to load vertices and elements */
            nk_buffer_init_fixed(&vbuf, vertices, (size_t)max_vertex_buffer);
            nk_buffer_init_fixed(&ebuf, elements, (size_t)max_element_buffer);
            nk_convert(&gles->ctx, &dev->cmds, &vbuf, &ebuf, &config);
        }

        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, &gles->ctx, &dev->cmds) {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor((GLint)(cmd->clip_rect.x * gles->fb_scale.x),
                      (GLint)((float)(gles->height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * gles->fb_scale.y),
                      (GLint)(cmd->clip_rect.w * gles->fb_scale.x), (GLint)(cmd->clip_rect.h * gles->fb_scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, (const void *)offset);
            offset += cmd->elem_count * sizeof(nk_draw_index);
        }
        nk_clear(&gles->ctx);
        nk_buffer_clear(&dev->cmds);
    }

    /* default OpenGL state */
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

/*NK_API void nk_gles32_char_callback(struct ANativeWindow *win, unsigned int codepoint) {
    struct nk_gles *gles = (struct nk_gles *)glesGetWindowUserPointer(win);
    if (gles->text_len < NK_GLES32_TEXT_MAX) gles->text[gles->text_len++] = codepoint;
}*/

/*NK_API void nk_gles32_key_callback(struct ANativeWindow *win, int key, int scancode, int action, int mods) {
    static int      insert_toggle = 0;
    struct nk_gles *gles          = (struct nk_gles *)glesGetWindowUserPointer(win);

    // convert gles_REPEAT to down (technically gles_RELEASE, gles_PRESS, gles_REPEAT are
    // already 0, 1, 2 but just to be clearer)
    nk_char a = (action == gles_RELEASE) ? nk_false : nk_true;

    NK_UNUSED(scancode);
    NK_UNUSED(mods);

    switch (key) {
    case gles_KEY_DELETE: gles->key_events[NK_KEY_DEL] = a; break;
    case gles_KEY_TAB: gles->key_events[NK_KEY_TAB] = a; break;
    case gles_KEY_BACKSPACE: gles->key_events[NK_KEY_BACKSPACE] = a; break;
    case gles_KEY_UP: gles->key_events[NK_KEY_UP] = a; break;
    case gles_KEY_DOWN: gles->key_events[NK_KEY_DOWN] = a; break;
    case gles_KEY_LEFT: gles->key_events[NK_KEY_LEFT] = a; break;
    case gles_KEY_RIGHT: gles->key_events[NK_KEY_RIGHT] = a; break;
    case gles_KEY_ESCAPE: gles->key_events[NK_KEY_TEXT_RESET_MODE] = a; break;

    case gles_KEY_LEFT_ALT:
    case gles_KEY_RIGHT_ALT: gles->key_events[NK_KEY_ALT] = a; break;
    case gles_KEY_PAGE_UP: gles->key_events[NK_KEY_SCROLL_UP] = a; break;
    case gles_KEY_PAGE_DOWN: gles->key_events[NK_KEY_SCROLL_DOWN] = a; break;
    case gles_KEY_F1: gles->key_events[NK_KEY_F1] = a; break;
    case gles_KEY_F2: gles->key_events[NK_KEY_F2] = a; break;
    case gles_KEY_F3: gles->key_events[NK_KEY_F3] = a; break;
    case gles_KEY_F4: gles->key_events[NK_KEY_F4] = a; break;
    case gles_KEY_F5: gles->key_events[NK_KEY_F5] = a; break;
    case gles_KEY_F6: gles->key_events[NK_KEY_F6] = a; break;
    case gles_KEY_F7: gles->key_events[NK_KEY_F7] = a; break;
    case gles_KEY_F8: gles->key_events[NK_KEY_F8] = a; break;
    case gles_KEY_F9: gles->key_events[NK_KEY_F9] = a; break;
    case gles_KEY_F10: gles->key_events[NK_KEY_F10] = a; break;
    case gles_KEY_F11: gles->key_events[NK_KEY_F11] = a; break;
    case gles_KEY_F12: gles->key_events[NK_KEY_F12] = a; break;

    case gles_KEY_C: gles->key_events[NK_KEY_COPY] = a; break;
    case gles_KEY_V: gles->key_events[NK_KEY_PASTE] = a; break;
    case gles_KEY_X: gles->key_events[NK_KEY_CUT] = a; break;
    case gles_KEY_Z: gles->key_events[NK_KEY_TEXT_UNDO] = a; break;
    case gles_KEY_R: gles->key_events[NK_KEY_TEXT_REDO] = a; break;
    case gles_KEY_B: gles->key_events[NK_KEY_TEXT_LINE_START] = a; break;
    case gles_KEY_E: gles->key_events[NK_KEY_TEXT_LINE_END] = a; break;
    case gles_KEY_A: gles->key_events[NK_KEY_TEXT_SELECT_ALL] = a; break;

    case gles_KEY_ENTER:
    case gles_KEY_KP_ENTER: gles->key_events[NK_KEY_ENTER] = a; break;
    case gles_KEY_INSERT:
        // Only switch on release to avoid repeat issues kind of confusing since we have to
        // negate it but we're already hacking it since Nuklear treats them as two separate
        // keys rather than a single toggle state
        if (!a) {
            insert_toggle = !insert_toggle;
            if (insert_toggle) {
                gles->key_events[NK_KEY_TEXT_INSERT_MODE] = !a;
                // gles->key_events[NK_KEY_TEXT_REPLACE_MODE] = a;
            } else {
                // gles->key_events[NK_KEY_TEXT_INSERT_MODE] = a;
                gles->key_events[NK_KEY_TEXT_REPLACE_MODE] = !a;
            }
        }
        break;
    default:;
    }
}*/

/*NK_API void nk_gflw3_scroll_callback(struct ANativeWindow *win, double xoff, double yoff) {
    struct nk_gles *gles = (struct nk_gles *)glesGetWindowUserPointer(win);
    (void)xoff;
    gles->scroll.x += (float)xoff;
    gles->scroll.y += (float)yoff;
}*/

/*NK_API void nk_gles32_mouse_button_callback(struct ANativeWindow *win, int button, int action, int mods) {
    struct nk_gles *gles = (struct nk_gles *)glesGetWindowUserPointer(win);
    double          x, y;
    NK_UNUSED(mods);
    if (button != gles_MOUSE_BUTTON_LEFT) return;
    glesGetCursorPos(win, &x, &y);
    if (action == gles_PRESS) {
        double dt = glesGetTime() - gles->last_button_click;
        if (dt > NK_gles_DOUBLE_CLICK_LO && dt < NK_gles_DOUBLE_CLICK_HI) {
            gles->is_double_click_down = nk_true;
            gles->double_click_pos     = nk_vec2((float)x, (float)y);
        }
        gles->last_button_click = glesGetTime();
    } else
        gles->is_double_click_down = nk_false;
}*/

/*NK_INTERN void nk_gles32_clipboard_paste(nk_handle usr, struct nk_text_edit *edit) {
    struct nk_gles *gles = (struct nk_gles *)usr.ptr;
    const char     *text = glesGetClipboardString(gles->win);
    if (text) nk_textedit_paste(edit, text, nk_strlen(text));
    (void)usr;
}

NK_INTERN void nk_gles32_clipboard_copy(nk_handle usr, const char *text, int len) {
    struct nk_gles *gles = (struct nk_gles *)usr.ptr;
    char           *str  = 0;
    if (!len) return;
    str = (char *)malloc((size_t)len + 1);
    if (!str) return;
    memcpy(str, text, (size_t)len);
    str[len] = '\0';
    glesSetClipboardString(gles->win, str);
    free(str);
}*/

//
NK_API struct nk_context *nk_gles32_init(struct nk_gles *gles, struct ANativeWindow *win, struct AAssetManager *assMan) {
    struct nk_font *font = NULL;
    gles->win            = win;

    LOGI("%s", "Entering nk_gles32_init");
    setup_default_font(gles, &font);

    LOGI("%s", "Initializing default context...");
    nk_init_default(&gles->ctx, &font->handle);

    gles->ctx.clip.copy     = NULL;
    gles->ctx.clip.paste    = NULL;
    gles->ctx.clip.userdata = nk_handle_ptr(gles);

    gles->last_button_click = 0;

    // Create GPU resources (shaders, VBO, EBO, font texture)
    LOGI("%s", "Creating GLES device resources...");
    nk_gles32_device_create(gles, assMan);

    gles->is_double_click_down = nk_false;
    gles->double_click_pos     = nk_vec2(0, 0);

    // Use standard Android monotonic clock instead of GLFW's time function
    gles->delta_time_seconds_last = android_get_time();
    LOGI("Finished nk_gles32_init successfully, ctx is %p", (void *)&gles->ctx);
    return &gles->ctx;
}

NK_API void nk_gles32_font_stash_begin(struct nk_gles *gles, struct nk_font_atlas **atlas) {
    nk_font_atlas_init_default(&gles->atlas);
    nk_font_atlas_begin(&gles->atlas);
    *atlas = &gles->atlas;
}

NK_API void nk_gles32_font_stash_end(struct nk_gles *gles) {
    const void *image;
    int         w, h;

    image = nk_font_atlas_bake(&gles->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_gles32_device_upload_atlas(gles, image, w, h);

    nk_font_atlas_end(&gles->atlas, nk_handle_id((int)gles->ogl.font_tex), &gles->ogl.tex_null);
    if (gles->atlas.default_font) nk_style_set_font(&gles->ctx, &gles->atlas.default_font->handle);
}

NK_API void nk_gles32_new_frame(struct nk_gles *gles) {
    int                i;
    struct nk_context *ctx     = &gles->ctx;
    nk_char           *k_state = gles->key_events;

    // update the timer
    float delta_time_now          = android_get_time();
    gles->ctx.delta_time_seconds  = delta_time_now - gles->delta_time_seconds_last;
    gles->delta_time_seconds_last = delta_time_now;

    // Get window dimensions using Android NDK instead of GLFW
    gles->width  = ANativeWindow_getWidth(gles->win);
    gles->height = ANativeWindow_getHeight(gles->win);

    // For standard Android surfaces, framebuffer matches window size
    gles->display_width  = gles->width;
    gles->display_height = gles->height;
    gles->fb_scale.x     = 1.0f;
    gles->fb_scale.y     = 1.0f;

    // Begin Nuklear input handling and feed any buffered text characters
    nk_input_begin(ctx);
    for (i = 0; i < gles->text_len; ++i) { nk_input_unicode(ctx, gles->text[i]); }

    if (k_state[NK_KEY_ENTER] >= 0) nk_input_key(ctx, NK_KEY_ENTER, k_state[NK_KEY_ENTER]);
    if (k_state[NK_KEY_BACKSPACE] >= 0) nk_input_key(ctx, NK_KEY_BACKSPACE, k_state[NK_KEY_BACKSPACE]);

    nk_input_end(&gles->ctx);

    /* clear after nk_input_end (-1 since we're doing up/down boolean) */
    memset(gles->key_events, -1, sizeof(gles->key_events));

    gles->text_len = 0;
    gles->scroll   = nk_vec2(0, 0);
}

NK_API void nk_gles32_shutdown(struct nk_gles *gles) {
    nk_font_atlas_clear(&gles->atlas);
    nk_free(&gles->ctx);
    nk_gles32_device_destroy(gles);
    memset(gles, 0, sizeof(*gles));
}
