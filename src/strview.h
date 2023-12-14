#ifndef HYPERTRACER_STRVIEW_H
#define HYPERTRACER_STRVIEW_H

#include <hypertracer.h>
#include <stddef.h>

static inline struct htStrView htStrView_empty(void) {
    struct htStrView result;
    result.buf = NULL;
    result.len = 0;
    return result;
}

static inline struct htStrView htStrView_new(const char *buf, size_t len) {
    struct htStrView result;
    result.buf = buf;
    result.len = len;
    return result;
}

#endif
