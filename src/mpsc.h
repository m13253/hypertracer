#ifndef HYPERTRACER_MPSC_H
#define HYPERTRACER_MPSC_H

#include <stdalign.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h>

#define HT_MPSC_ALIGN 64

struct htMpsc {
    alignas(HT_MPSC_ALIGN) mtx_t mtx_read;
    mtx_t mtx_write;
    cnd_t cnd_read;
    cnd_t cnd_write;

    atomic_size_t size;
    atomic_size_t back;
    atomic_bool is_popping;

    alignas(HT_MPSC_ALIGN) void *data;
    size_t cap;
    size_t num_str_columns;
    size_t front;
};

struct htString;
struct timespec;

void htMpsc_new(struct htMpsc *self, size_t cap, size_t num_str_columns);
void htMpsc_free(struct htMpsc *self);
void htMpsc_push(struct htMpsc *self, const struct timespec *time, struct htString *columns);
bool htMpsc_pop(struct htMpsc *self, struct timespec *time, struct htString *columns);
void htMpsc_close(struct htMpsc *self);
static inline size_t htMpsc_num_str_columns(const struct htMpsc *self) {
    return self->num_str_columns;
}

#endif
