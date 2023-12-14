#include "array.h"
#include "strbuilder.h"
#include <hypertracer.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    HT_ARRAY_MIN_CAP = 4,
};

struct htArray htArray_new(void) {
    struct htArray self;
    self.data = NULL;
    self.len = 0;
    self.cap = 0;
    return self;
}

struct htArray htArray_with_capacity(size_t cap) {
    struct htArray self;
    if (cap == 0) {
        self.data = NULL;
    } else {
        if (cap > SIZE_MAX / sizeof self.data[0]) {
            fprintf(stderr, "panic: htArray_with_capacity: capacity %zu is too large\n", cap);
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

void htArray_free(struct htArray *self) {
    for (size_t i = 0; i < self->len; i++) {
        free((char *) self->data[i].buf);
    }
    free(self->data);
}

void htArray_clear(struct htArray *self) {
    for (size_t i = 0; i < self->len; i++) {
        free((char *) self->data[i].buf);
    }
    self->len = 0;
}

static void htArray_grow(struct htArray *self) {
    if (self->cap == 0) {
        self->cap = HT_ARRAY_MIN_CAP;
    } else {
        size_t limit = SIZE_MAX / sizeof self->data[0] - self->cap;
        if (limit == 0) {
            fprintf(stderr, "panic: htArray_grow: capacity %zu is too large\n", self->cap);
            abort();
        } else if (limit <= self->cap) {
            self->cap = SIZE_MAX / sizeof self->data[0];
        } else {
            self->cap *= 2;
        }
    }
    struct htStrView *data = realloc(self->data, self->cap * sizeof self->data[0]);
    if (!data) {
        free(self->data);
        abort();
    }
    self->data = data;
}

void htArray_push(struct htArray *self, struct htStrBuilder *str) {
    if (self->len == self->cap) {
        htArray_grow(self);
    }
    htStrBuilder_shrink(str);
    self->data[self->len].buf = str->buf;
    self->data[self->len].len = str->len;
    self->len++;
}

bool htArray_try_push(struct htArray *self, struct htStrBuilder *str) {
    if (self->len == self->cap) {
        return false;
    }
    htStrBuilder_shrink(str);
    self->data[self->len].buf = str->buf;
    self->data[self->len].len = str->len;
    self->len++;
    return true;
}

void htArray_shrink(struct htArray *self) {
    if (self->cap == self->len) {
        return;
    }
    self->cap = self->len;
    if (self->cap == 0) {
        free(self->data);
        self->data = NULL;
    } else {
        struct htStrView *data = realloc(self->data, self->cap * sizeof self->data[0]);
        if (!data) {
            free(self->data);
            abort();
        }
        self->data = data;
    }
}
