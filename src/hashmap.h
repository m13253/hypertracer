#ifndef HT_HASHMAP_H
#define HT_HASHMAP_H

#include <hypetrace.h>
#include <stdbool.h>
#include <stddef.h>

struct HTHashmap {
    struct HTHashmap_Entry *data;
    size_t num_buckets;
};

struct HTHashmap_Entry {
    bool valid;
    struct HTStrView key;
    size_t value;
};

struct HTHashmap HTHashmap_new(size_t num_elements);
void HTHashmap_drop(struct HTHashmap *self);
size_t HTHashmap_try_set(
    struct HTHashmap *self,
    const char *key, size_t key_len,
    size_t value
);
bool HTHashmap_get(
    const struct HTHashmap *self,
    size_t *out_value,
    const char *key, size_t key_len
);

#endif
