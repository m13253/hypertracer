#include "strbuf.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    HT_STRBUF_MIN_CAP = 8,
};

struct HTStrBuf HTStrBuf_new(void) {
    struct HTStrBuf self;
    self.buf = NULL;
    self.len = 0;
    self.cap = 0;
    return self;
}

void HTStrBuf_free(struct HTStrBuf *self) {
    free(self->buf);
}

static void HTStrBuf_grow(struct HTStrBuf *self) {
    if (self->cap == 0) {
        self->cap = HT_STRBUF_MIN_CAP;
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

void HTStrBuf_push(struct HTStrBuf *self, char ch) {
    if (self->len == self->cap) {
        HTStrBuf_grow(self);
    }
    self->buf[self->len] = ch;
    self->len++;
}

void HTStrBuf_shrink(struct HTStrBuf *self) {
    if (self->cap == self->len) {
        return;
    }
    self->cap = self->len;
    if (self->cap == 0) {
        free(self->buf);
        self->buf = NULL;
    } else {
        self->buf = realloc(self->buf, self->cap);
        if (!self->buf) {
            abort();
        }
    }
}
