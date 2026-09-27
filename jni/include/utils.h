#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdlib.h>

// Android stuff start
#include <android/log.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#define LOG_LEVEL ANDROID_LOG_INFO
#define LOG_TAG "linearconcalc"

// NOLINTBEGIN
#define LOGI(fmt, ...) log_internal("\x1b[32m ", ANDROID_LOG_INFO, LOG_TAG, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#define LOGE(fmt, ...) log_internal("\x1b[91m ", ANDROID_LOG_ERROR, LOG_TAG, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#define LOGV(fmt, ...) log_internal("\x1b[90m ", ANDROID_LOG_VERBOSE, LOG_TAG, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
// NOLINTEND

//  Android stuff end

typedef const char *const str_ref;
typedef void (*Destructor)(void *);

typedef struct {
    void       *obj;
    const char *name;
    Destructor  func;
} Freeable;

// NOLINTBEGIN
#define TO_DFREE(var) ((Freeable){var, #var, (Destructor)free})
#define TO_FREE(var, func) ((Freeable){var, #var, (Destructor)(void*)func})

#define FREE_ALL(...) ( \
    free_all_impl((Freeable[]){ __VA_ARGS__ }, sizeof((Freeable[]){ __VA_ARGS__ }) / sizeof(Freeable)) \
)
// NOLINTEND

void free_all_impl(Freeable *objs, size_t obj_num);
void log_internal(str_ref color, int priority, const char *tag, str_ref file, int line, str_ref function, str_ref fmt, ...)
    __attribute__((format(printf, 7, 8)));

#endif
