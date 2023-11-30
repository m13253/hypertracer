#include "strbuilder.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    HT_STR_BUILDER_MIN_CAP = 8,
};

struct HTStrBuilder HTStrBuilder_new(void) {
    struct HTStrBuilder self;
    self.buf = NULL;
    self.len = 0;
    self.cap = 0;
    return self;
}

void HTStrBuilder_free(struct HTStrBuilder *self) {
    free(self->buf);
}

static void HTStrBuilder_grow(struct HTStrBuilder *self) {
    if (self->cap == 0) {
        self->cap = HT_STR_BUILDER_MIN_CAP;
    } else {
        size_t limit = SIZE_MAX - self->cap;
        if (limit == 0) {
            abort();
        } else if (limit <= self->cap) {
            self->cap = SIZE_MAX;
        } else {
            self->cap *= 2;
        }
    }
    self->buf = realloc(self->buf, self->cap);
    if (!self->buf) {
        abort();
    }
}

void HTStrBuilder_push(struct HTStrBuilder *self, char ch) {
    if (self->len == self->cap) {
        HTStrBuilder_grow(self);
    }
    self->buf[self->len] = ch;
    self->len++;
}
