#include "string.h"
#include <stdlib.h>

struct HTString HTString_from_HTStrBuilder(char *buf, size_t len, size_t cap) {
    struct HTString self;
    if (len == 0) {
        free(buf);
        self.buf = NULL;
    } else if (len == cap) {
        self.buf = buf;
    } else {
        char *new_buf = realloc(buf, len);
        if (!new_buf) {
            free(buf);
            abort();
        }
        self.buf = new_buf;
    }
    self.len = len;
    return self;
}

void HTString_free(struct HTString *self) {
    free(self->buf);
}
