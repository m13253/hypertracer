#ifndef HT_ARRAY_H
#define HT_ARRAY_H

#include <stdbool.h>
#include <stddef.h>

struct HTArray {
    struct HTStrView *data;
    size_t len;
    size_t cap;
};

struct HTStrBuilder;

struct HTArray HTArray_new(void);
struct HTArray HTArray_with_capacity(size_t cap);
void HTArray_free(struct HTArray *self);
void HTArray_clear(struct HTArray *self);
void HTArray_push(struct HTArray *self, struct HTStrBuilder *str);
bool HTArray_try_push(struct HTArray *self, struct HTStrBuilder *str);
void HTArray_shrink(struct HTArray *self);

#endif
