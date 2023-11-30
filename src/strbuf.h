#ifndef HT_STRBUF_H
#define HT_STRBUF_H

#include <stddef.h>

struct HTStrBuf {
    char *buf;
    size_t len;
    size_t cap;
};

struct HTStrBuf HTStrBuf_new(void);
void HTStrBuf_drop(struct HTStrBuf *self);
void HTStrBuf_push(struct HTStrBuf *self, char ch);
void HTStrBuf_shrink(struct HTStrBuf *self);

#endif
