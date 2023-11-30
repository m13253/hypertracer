#include "string.h"
#include <stdlib.h>

struct HTString HTString_from_HTStrBuf(char *buf, size_t len, size_t cap) {
    if (len == 0) {
        free(buf);
        return HTString_from_HTStrView(NULL, 0);
    }
    struct HTString self;
    if (len == cap) {
        self.buf = buf;
    } else {
        self.buf = realloc(buf, len);
        if (!self.buf) {
            abort();
        }
    }
    self.len = len;
    self.owned = true;
    return self;
}

struct HTString HTString_from_HTStrView(const char *buf, size_t len) {
    struct HTString self;
    self.buf = (char *) buf;
    self.len = len;
    self.owned = false;
    return self;
}

void HTString_drop(struct HTString *self) {
    if (self->owned) {
        free(self->buf);
    }
}
