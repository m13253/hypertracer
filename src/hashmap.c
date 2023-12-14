#include "hashmap.h"
#include <hypertracer.h>
#include <siphash.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct htHashmapEntry {
    struct htStrView key;
    size_t value;
    bool valid;
};

struct htHashmap htHashmap_new(size_t num_elements) {
    struct htHashmap self;
    size_t limit = SIZE_MAX - num_elements;
    if (limit == 0) {
        fprintf(stderr, "panic: htHashmap_new: size %zu is too large\n", num_elements);
        abort();
    } else if (limit <= num_elements / 3) {
        self.num_buckets = SIZE_MAX;
    } else {
        self.num_buckets = num_elements + num_elements / 3;
    }
    if (self.num_buckets == 0) {
        self.num_buckets = 1;
    }
    self.data = calloc(self.num_buckets, sizeof self.data[0]);
    if (!self.data) {
        abort();
    }
    return self;
}

void htHashmap_free(struct htHashmap *self) {
    free(self->data);
}

static uint64_t htHashmap_hash(const char *key, size_t key_len) {
    const uint8_t secret[16] = {0};
    uint8_t hash_buf[8];
    siphash(key_len == 0 ? "" : key, key_len, secret, hash_buf, sizeof hash_buf);
    return (
        (uint64_t) hash_buf[0] |
        ((uint64_t) hash_buf[1] << 8) |
        ((uint64_t) hash_buf[2] << 16) |
        ((uint64_t) hash_buf[3] << 24) |
        ((uint64_t) hash_buf[4] << 32) |
        ((uint64_t) hash_buf[5] << 40) |
        ((uint64_t) hash_buf[6] << 48) |
        ((uint64_t) hash_buf[7] << 56)
    );
}

size_t htHashmap_try_set(struct htHashmap *self, const char *key, size_t key_len, size_t value) {
    size_t hash = (size_t) (htHashmap_hash(key, key_len) % self->num_buckets);
    while (self->data[hash].valid) {
        if (
            self->data[hash].key.len == key_len &&
            (key_len == 0 || memcmp(self->data[hash].key.buf, key, key_len) == 0)
        ) {
            return self->data[hash].value;
        }
        hash++;
        if (hash == self->num_buckets) {
            hash = 0;
        }
    }
    self->data[hash].key.buf = key;
    self->data[hash].key.len = key_len;
    self->data[hash].value = value;
    self->data[hash].valid = true;
    return value;
}

bool htHashmap_get(const struct htHashmap *self, size_t *out_value, const char *key, size_t key_len) {
    size_t hash = (size_t) (htHashmap_hash(key, key_len) % self->num_buckets);
    while (self->data[hash].valid) {
        if (
            self->data[hash].key.len == key_len &&
            (key_len == 0 || memcmp(self->data[hash].key.buf, key, key_len) == 0)
        ) {
            *out_value = self->data[hash].value;
            return true;
        }
        hash++;
        if (hash == self->num_buckets) {
            hash = 0;
        }
    }
    return false;
}
