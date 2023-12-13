#ifndef HT_STRBUILDER_H
#define HT_STRBUILDER_H

#include <stddef.h>

struct HTStrBuilder {
    char *buf;
    size_t len;
    size_t cap;
};

struct HTStrBuilder HTStrBuilder_new(void);
void HTStrBuilder_free(struct HTStrBuilder *self);
void HTStrBuilder_push(struct HTStrBuilder *self, char ch);
void HTStrBuilder_shrink(struct HTStrBuilder *self);

#endif
