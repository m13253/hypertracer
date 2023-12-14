#include "strbuilder.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    HT_STR_BUILDER_MIN_CAP = 8,
};

struct htStrBuilder htStrBuilder_new(void) {
    struct htStrBuilder self;
    self.buf = NULL;
    self.len = 0;
    self.cap = 0;
    return self;
}

void htStrBuilder_free(struct htStrBuilder *self) {
    free(self->buf);
}

static void htStrBuilder_grow(struct htStrBuilder *self) {
    if (self->cap == 0) {
        self->cap = HT_STR_BUILDER_MIN_CAP;
    } else {
        size_t limit = SIZE_MAX - self->cap;
        if (limit == 0) {
            fprintf(stderr, "panic: htStrBuilder_grow: capacity %zu is too large\n", self->cap);
            abort();
        } else if (limit <= self->cap) {
            self->cap = SIZE_MAX;
        } else {
            self->cap *= 2;
        }
    }
    char *buf = realloc(self->buf, self->cap);
    if (!buf) {
        free(self->buf);
        abort();
    }
    self->buf = buf;
}

void htStrBuilder_push(struct htStrBuilder *self, char ch) {
    if (self->len == self->cap) {
        htStrBuilder_grow(self);
    }
    self->buf[self->len] = ch;
    self->len++;
}

void htStrBuilder_shrink(struct htStrBuilder *self) {
    if (self->len == 0) {
        free(self->buf);
        self->buf = NULL;
    } else if (self->len != self->cap) {
        char *buf = realloc(self->buf, self->len);
        if (!buf) {
            free(self->buf);
            abort();
        }
        self->buf = buf;
        self->cap = self->len;
    }
}
