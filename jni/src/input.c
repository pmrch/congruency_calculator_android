#include "android_native_app_glue.h"

#include "render.h"
#include "utils.h"

int32_t handleInput(struct android_app *app, AInputEvent *evt) {
    UserData *luserData = (UserData *)app->userData;
    int32_t   type      = AInputEvent_getType(evt);

    int   action_masked = 0;
    float x = 0.0f, y = 0.0f;

    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = 0;

        // clang-format off
        LOGI("TOUCH action=%d x=%f y=%f", AMotionEvent_getAction(evt) & AMOTION_EVENT_ACTION_MASK, AMotionEvent_getX(evt, 0), AMotionEvent_getY(evt, 0));
        LOGV("%s", "Motion input detected");
        // clang-format on

        action        = AMotionEvent_getAction(evt);
        action_masked = action & AMOTION_EVENT_ACTION_MASK;
        LOGV("Got action type: %d", action);

        x = AMotionEvent_getX(evt, 0);
        y = AMotionEvent_getY(evt, 0);
        LOGV("Got movement action at x=%f, y=%f", x, y);

        if (action_masked == AMOTION_EVENT_ACTION_DOWN || action_masked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            LOGV("Pressed at x=%f y=%f", x, y);
            nk_input_motion(luserData->nkCtx, (int)x, (int)y);
            nk_input_button(luserData->nkCtx, NK_BUTTON_LEFT, (int)x, (int)y, 1);
            return 1;
        }

        if (action_masked == AMOTION_EVENT_ACTION_UP || action_masked == AMOTION_EVENT_ACTION_POINTER_UP) {
            LOGV("Released at x=%f y=%f", x, y);
            nk_input_motion(luserData->nkCtx, (int)x, (int)y);
            nk_input_button(luserData->nkCtx, NK_BUTTON_LEFT, (int)x, (int)y, 0);
            return 1;
        }

        if (action_masked == AMOTION_EVENT_ACTION_MOVE) {
            nk_input_motion(luserData->nkCtx, (int)x, (int)y);
            return 1;
        }
    }

    LOGI("Got input type: %d", type);
    return 0;
}
