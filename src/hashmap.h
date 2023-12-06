#ifndef HT_HASHMAP_H
#define HT_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>

struct HTHashmap {
    struct HTHashmapEntry *data;
    size_t num_buckets;
};

struct HTHashmap HTHashmap_new(size_t num_elements);
void HTHashmap_free(struct HTHashmap *self);
size_t HTHashmap_try_set(struct HTHashmap *self, const char *key, size_t key_len, size_t value);
bool HTHashmap_get(const struct HTHashmap *self, size_t *out_value, const char *key, size_t key_len);

#endif
