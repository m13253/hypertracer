#include "array.h"
#include "string.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    HT_ARRAY_MIN_CAP = 4,
};

struct HTArray HTArray_new(void) {
    struct HTArray self;
    self.data = NULL;
    self.len = 0;
    self.cap = 0;
    return self;
}

struct HTArray HTArray_with_capacity(size_t cap) {
    struct HTArray self;
    if (cap == 0) {
        self.data = NULL;
    } else {
        if (cap > SIZE_MAX / sizeof self.data[0]) {
            abort();
        }
        self.data = malloc(cap * sizeof self.data[0]);
        if (!self.data) {
            abort();
        }
    }
    self.len = 0;
    self.cap = cap;
    return self;
}

void HTArray_free(struct HTArray *self) {
    for (size_t i = 0; i < self->len; i++) {
        HTString_free(&self->data[i]);
    }
    free(self->data);
}

void HTArray_clear(struct HTArray *self) {
    for (size_t i = 0; i < self->len; i++) {
        HTString_free(&self->data[i]);
    }
    self->len = 0;
}

static void HTArray_grow(struct HTArray *self) {
    if (self->cap == 0) {
        self->cap = HT_ARRAY_MIN_CAP;
    } else {
        size_t limit = SIZE_MAX / sizeof self->data[0] - self->cap;
        if (limit == 0) {
            abort();
        } else if (limit <= self->cap) {
            self->cap = SIZE_MAX / sizeof self->data[0];
        } else {
            self->cap *= 2;
        }
    }
    self->data = realloc(self->data, self->cap * sizeof self->data[0]);
    if (!self->data) {
        abort();
    }
}

void HTArray_push(struct HTArray *self, char *value, size_t value_len, size_t value_cap) {
    if (self->len == self->cap) {
        HTArray_grow(self);
    }
    self->data[self->len] = HTString_from_HTStrBuilder(value, value_len, value_cap);
    self->len++;
}

bool HTArray_try_push(struct HTArray *self, char *value, size_t value_len, size_t value_cap) {
    if (self->len == self->cap) {
        return false;
    }
    self->data[self->len] = HTString_from_HTStrBuilder(value, value_len, value_cap);
    self->len++;
    return true;
}

void HTArray_shrink(struct HTArray *self) {
    if (self->cap == self->len) {
        return;
    }
    self->cap = self->len;
    if (self->cap == 0) {
        free(self->data);
        self->data = NULL;
    } else {
        self->data = realloc(self->data, self->cap * sizeof self->data[0]);
        if (!self->data) {
            abort();
        }
    }
}
