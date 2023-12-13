#ifndef HT_MPSC_H
#define HT_MPSC_H

#include <stdalign.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h>
#include <time.h>

#define HT_MPSC_ALIGN 64

struct HTMpsc {
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

struct HTString;

void HTMpsc_new(struct HTMpsc *self, size_t cap, size_t num_str_columns);
void HTMpsc_free(struct HTMpsc *self);
void HTMpsc_push(struct HTMpsc *self, const struct timespec *time, const struct HTString *columns);
bool HTMpsc_pop(struct HTMpsc *self, struct timespec *time, struct HTString *columns);
void HTMpsc_close(struct HTMpsc *self);
static inline size_t HTMpsc_num_str_columns(const struct HTMpsc *self) {
    return self->num_str_columns;
}

#endif
