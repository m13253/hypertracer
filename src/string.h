#ifndef HT_STRING_H
#define HT_STRING_H

#include <stdbool.h>
#include <stddef.h>

struct HTString {
    char *buf;
    size_t len;
    bool owned;
};

struct HTString HTString_from_HTStrBuf(char *buf, size_t len, size_t cap);
struct HTString HTString_from_HTStrView(const char *buf, size_t len);
void HTString_drop(struct HTString *self);

#endif
