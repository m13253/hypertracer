#ifndef HYPERTRACER_HASHMAP_H
#define HYPERTRACER_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>

struct htHashmap {
    struct htHashmapEntry *data;
    size_t num_buckets;
};

struct htHashmap htHashmap_new(size_t num_elements);
void htHashmap_free(struct htHashmap *self);
size_t htHashmap_try_set(struct htHashmap *self, const char *key, size_t key_len, size_t value);
bool htHashmap_get(const struct htHashmap *self, size_t *out_value, const char *key, size_t key_len);

#endif
