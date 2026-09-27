#ifndef UI_H
#define UI_H

#include "nuklear_android_gles32.h"

typedef struct {
    int32_t width, height;
    int32_t min_width, max_width;
    int32_t min_height, max_height;
} WindowDimensions;

void drawUi(struct nk_context *nkCtx, WindowDimensions *dims);

#endif
