#ifndef HT_STRVIEW_H
#define HT_STRVIEW_H

#include <hypertracer.h>
#include <stddef.h>

static inline struct HTStrView HTStrView_empty(void) {
    struct HTStrView result;
    result.buf = NULL;
    result.len = 0;
    return result;
}

static inline struct HTStrView HTStrView_new(const char *buf, size_t len) {
    struct HTStrView result;
    result.buf = buf;
    result.len = len;
    return result;
}

#endif
