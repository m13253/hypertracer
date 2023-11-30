#ifndef HT_STRING_H
#define HT_STRING_H

#include <stdbool.h>
#include <stddef.h>

struct HTString {
    char *buf;
    size_t len;
};

struct HTString HTString_from_HTStrBuilder(char *buf, size_t len, size_t cap);
void HTString_free(struct HTString *self);

#endif
