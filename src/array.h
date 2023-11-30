#ifndef HT_ARRAY_H
#define HT_ARRAY_H

#include <stdbool.h>
#include <stddef.h>

struct HTArray {
    struct HTString *data;
    size_t len;
    size_t cap;
};

struct HTArray HTArray_new(void);
struct HTArray HTArray_with_capacity(size_t cap);
void HTArray_drop(struct HTArray *self);
void HTArray_clear(struct HTArray *self);
void HTArray_push(struct HTArray *self, char *value, size_t value_len, size_t value_cap);
bool HTArray_try_push(struct HTArray *self, char *value, size_t value_len, size_t value_cap);
void HTArray_shrink(struct HTArray *self);

#endif
