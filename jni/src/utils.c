#include "utils.h"
#include <string.h>

void free_all_impl(Freeable *objs, size_t obj_num) {
    const void *old_addr = NULL;

    if (objs == NULL) {
        LOGE("%s", "There were no objects to free");
        return;
    }

    for (size_t i = 0; i < obj_num; i++) {
        if (objs[i].obj == NULL) { continue; } // I don't care if one obj is NULL, doesn't matter
        if (objs[i].func == NULL) {
            LOGE("Destructor was NULL for objs[%zu]", i);
            continue;
        }

        old_addr = (const void *)objs[i].obj;
        objs[i].func(objs[i].obj);
        LOGV("Freed %s at address %p", objs[i].name, old_addr);
    }
}

void log_internal(str_ref color, int priority, const char *tag, str_ref file, int line, str_ref function, str_ref fmt, ...) {
    va_list args;
    int     bytesWritten = 0;

    const char *level        = "unset";
    char        buffer[1024] = {0};
    size_t      bufsize      = sizeof(buffer);

    if (priority < LOG_LEVEL) { return; }
    if (priority == ANDROID_LOG_ERROR) { level = "ERROR"; }
    if (priority == ANDROID_LOG_INFO) { level = "INFO"; }
    if (priority == ANDROID_LOG_VERBOSE) { level = "VERBOSE"; }

    bytesWritten += snprintf(buffer, bufsize, "%s[%s]\x1b[0m %s:%d:%s: ", color, level, file, line, function);
    if (bytesWritten <= 0) {
        __android_log_write(ANDROID_LOG_ERROR, tag, "Failed to write to log!");
        return;
    }

    va_start(args, fmt);
    bytesWritten = vsnprintf(buffer + bytesWritten, bufsize - (size_t)bytesWritten, fmt, args);
    va_end(args);

    if (bytesWritten <= 0) {
        __android_log_write(ANDROID_LOG_ERROR, tag, "Failed to write to log!");
        return;
    }

    __android_log_write(priority, tag, buffer);
}
