#ifndef HYPERTRACER_STRBUILDER_H
#define HYPERTRACER_STRBUILDER_H

#include <stddef.h>

struct htStrBuilder {
    char *buf;
    size_t len;
    size_t cap;
};

struct htStrBuilder htStrBuilder_new(void);
void htStrBuilder_free(struct htStrBuilder *self);
void htStrBuilder_push(struct htStrBuilder *self, char ch);
void htStrBuilder_shrink(struct htStrBuilder *self);

#endif
