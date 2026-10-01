#include <android/input.h>

#include "ui.h"
#include "utils.h"

#define ROW_HEIGHT 75

void drawUi(struct nk_context *nkCtx, WindowDimensions *dims) {
    const char *const windowTitle  = "Linear Congruency Calculator";
    const float       windowHeight = (float)dims->height / 3.0f;

    nk_flags       windowFlags  = NK_WINDOW_BORDER | NK_WINDOW_TITLE;
    struct nk_rect windowBounds = nk_rect(0, (float)dims->max_height - windowHeight, (float)dims->max_width, windowHeight);

    // Top is smaller than bottom
    if (nk_begin(nkCtx, windowTitle, windowBounds, windowFlags)) {
        /* Display */
        struct nk_rect bounds = nk_window_get_bounds(nkCtx);
        LOGI("NK WINDOW BOUNDS: x=%f y=%f w=%f h=%f", bounds.x, bounds.y, bounds.w, bounds.h);

        nk_layout_row_dynamic(nkCtx, ROW_HEIGHT, 4);

        // clang-format off
        if (nk_button_label(nkCtx, "7")) { LOGI("%s", "******** 7 CLICKED ********"); }
        struct nk_rect r = nk_widget_bounds(nkCtx);
        LOGI("BUTTON 7: x=%f y=%f w=%f h=%f", r.x, r.y, r.w, r.h);

        if (nk_button_label(nkCtx, "8")) {  }
        if (nk_button_label(nkCtx, "9")) { }
        if (nk_button_label(nkCtx, "<-")) { }

        nk_layout_row_dynamic(nkCtx, ROW_HEIGHT, 4);
        if (nk_button_label(nkCtx, "4")) { }
        if (nk_button_label(nkCtx, "5")) { }
        if (nk_button_label(nkCtx, "6")) { }
        if (nk_button_label(nkCtx, "OK")) { }

        nk_layout_row_dynamic(nkCtx, ROW_HEIGHT, 4);
        if (nk_button_label(nkCtx, "1")) { }
        if (nk_button_label(nkCtx, "2")) { }
        if (nk_button_label(nkCtx, "3")) { }
        if (nk_button_label(nkCtx, "+")) { }

        nk_layout_row_dynamic(nkCtx, ROW_HEIGHT, 4);
        if (nk_button_label(nkCtx, "0")) { }
        if (nk_button_label(nkCtx, ".")) { }
        if (nk_button_label(nkCtx, "=")) { }
        if (nk_button_label(nkCtx, "C")) {  }

        // clang-format on
    }

    nk_end(nkCtx);
}
