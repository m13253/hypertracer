#ifndef HYPERTRACER_ARRAY_H
#define HYPERTRACER_ARRAY_H

#include <stdbool.h>
#include <stddef.h>

struct htArray {
    struct htStrView *data;
    size_t len;
    size_t cap;
};

struct htStrBuilder;

struct htArray htArray_new(void);
struct htArray htArray_with_capacity(size_t cap);
void htArray_free(struct htArray *self);
void htArray_clear(struct htArray *self);
void htArray_push(struct htArray *self, struct htStrBuilder *str);
bool htArray_try_push(struct htArray *self, struct htStrBuilder *str);
void htArray_shrink(struct htArray *self);

#endif
