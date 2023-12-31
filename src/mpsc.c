#include "mpsc.h"
#include <hypertracer.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

enum htMpscFlag {
    HT_MPSC_EMPTY = 0,
    HT_MPSC_VALUE = 1,
    HT_MPSC_EOF = 2,
};

struct htMpscRow {
    atomic_uint_fast8_t flag;
    struct timespec time;
    struct htString columns[];
};

static inline size_t htMpsc_size_per_row(const struct htMpsc *self) {
    return sizeof(struct htMpscRow) + self->num_str_columns * sizeof(struct htString);
}

static struct htMpscRow *htMpsc_get_row(const struct htMpsc *self, size_t row) {
    return (struct htMpscRow *) &((char *) self->data)[row * htMpsc_size_per_row(self)];
}

void htMpsc_new(struct htMpsc *self, size_t cap, size_t num_str_columns) {
    if (num_str_columns > (SIZE_MAX - sizeof(struct htMpscRow)) / sizeof(struct htString)) {
        fprintf(stderr, "panic: htMpsc_new: number of columns %zu is too large\n", num_str_columns);
        abort();
    }
    self->num_str_columns = num_str_columns;
    size_t size_per_row = htMpsc_size_per_row(self);
    if (cap == 0) {
        fprintf(stderr, "panic: htMpsc_new: capacity %zu is too small\n", cap);
        abort();
    } else if (cap > SIZE_MAX / size_per_row) {
        fprintf(stderr, "panic: htMpsc_new: capacity %zu is too large\n", cap);
        abort();
    }
    self->data = malloc(cap * size_per_row);
    if (!self->data) {
        abort();
    }
    for (size_t i = 0; i < cap; i++) {
        atomic_init(&htMpsc_get_row(self, i)->flag, HT_MPSC_EMPTY);
    }

    if (mtx_init(&self->mtx_read, mtx_plain) != thrd_success) {
        fputs("panic: htMpsc_new: failed to create the mutex\n", stderr);
        free(self->data);
        abort();
    }
    if (mtx_init(&self->mtx_write, mtx_plain) != thrd_success) {
        fputs("panic: htMpsc_new: failed to create the mutex\n", stderr);
        free(self->data);
        mtx_destroy(&self->mtx_read);
        abort();
    }
    if (cnd_init(&self->cnd_read) != thrd_success) {
        fputs("panic: htMpsc_new: failed to create the conditional variable\n", stderr);
        free(self->data);
        mtx_destroy(&self->mtx_read);
        mtx_destroy(&self->mtx_write);
        abort();
    }
    if (cnd_init(&self->cnd_write) != thrd_success) {
        fputs("panic: htMpsc_new: failed to create the conditional variable\n", stderr);
        free(self->data);
        mtx_destroy(&self->mtx_read);
        mtx_destroy(&self->mtx_write);
        cnd_destroy(&self->cnd_read);
        abort();
    }
    atomic_init(&self->size, 0);
    atomic_init(&self->back, 0);
    atomic_init(&self->is_popping, false);
    self->cap = cap;
    self->front = 0;
}

void htMpsc_free(struct htMpsc *self) {
    mtx_destroy(&self->mtx_read);
    mtx_destroy(&self->mtx_write);
    cnd_destroy(&self->cnd_read);
    cnd_destroy(&self->cnd_write);
    free(self->data);
}

static inline struct htMpscRow *htMpsc_before_push(struct htMpsc *self) {
    size_t size = atomic_load_explicit(&self->size, memory_order_relaxed);
    bool exchanged = false;
    while (!exchanged) {
        if (size >= self->cap) {
            // Queue is full
            if (mtx_lock(&self->mtx_write) != thrd_success) {
                fputs("panic: htMpsc_push: failed to lock the mutex\n", stderr);
                abort();
            }
            size = atomic_load_explicit(&self->size, memory_order_relaxed);
            while (!exchanged) {
                if (size >= self->cap) {
                    if (cnd_wait(&self->cnd_write, &self->mtx_write) != thrd_success) {
                        fputs("panic: htMpsc_push: failed to wait on the conditional variable\n", stderr);
                        mtx_unlock(&self->mtx_write);
                        abort();
                    }
                    size = atomic_load_explicit(&self->size, memory_order_relaxed);
                } else {
                    // Acquire a slot (slow path)
                    exchanged = atomic_compare_exchange_weak_explicit(&self->size, &size, size + 1, memory_order_acquire, memory_order_relaxed);
                }
            }
            if (mtx_unlock(&self->mtx_write) != thrd_success) {
                fputs("panic: htMpsc_push: failed to unlock the mutex\n", stderr);
                abort();
            }
        } else {
            // Acquire a slot (fast path)
            exchanged = atomic_compare_exchange_weak_explicit(&self->size, &size, size + 1, memory_order_acquire, memory_order_relaxed);
        }
    }
    // Allocate an index from back
    size_t back = atomic_fetch_add_explicit(&self->back, 1, memory_order_relaxed);
    if (back >= self->cap) {
        size_t back_unwrap = back;
        back %= self->cap;
        while (!atomic_compare_exchange_weak_explicit(&self->back, &back_unwrap, back_unwrap % self->cap, memory_order_relaxed, memory_order_relaxed) && back_unwrap >= self->cap) {
        }
    }

    return htMpsc_get_row(self, back);
}

static inline void htMpsc_after_push(struct htMpsc *self) {
    // Wake up the reading thread
    if (atomic_load_explicit(&self->is_popping, memory_order_relaxed)) {
        // Locking makes sure the reading thread is either waiting or already finished
        if (mtx_lock(&self->mtx_read) != thrd_success) {
            fputs("panic: htMpsc_push: failed to lock the mutex\n", stderr);
            abort();
        }
        if (cnd_signal(&self->cnd_read) != thrd_success) {
            fputs("panic: htMpsc_push: failed to signal the conditional variable\n", stderr);
            mtx_unlock(&self->mtx_read);
            abort();
        }
        if (mtx_unlock(&self->mtx_read) != thrd_success) {
            fputs("panic: htMpsc_push: failed to unlock the mutex\n", stderr);
            abort();
        }
    }
}

void htMpsc_push(struct htMpsc *self, const struct timespec *time, struct htString *columns) {
    struct htMpscRow *row = htMpsc_before_push(self);
    row->time = *time;
    memcpy(row->columns, columns, htMpsc_num_str_columns(self) * sizeof row->columns[0]);
    uint_fast8_t flag_invalid = HT_MPSC_EMPTY;
    if (!atomic_compare_exchange_strong_explicit(&row->flag, &flag_invalid, HT_MPSC_VALUE, memory_order_release, memory_order_relaxed)) {
        fputs("panic: htMpsc_push: memory corruption\n", stderr);
        abort();
    }
    htMpsc_after_push(self);
}

bool htMpsc_pop(struct htMpsc *self, struct timespec *time, struct htString *columns) {
    struct htMpscRow *row = htMpsc_get_row(self, self->front);

    if (mtx_lock(&self->mtx_read) != thrd_success) {
        fputs("panic: htMpsc_pop: failed to lock the mutex\n", stderr);
        abort();
    }
    // Tell the writing thread we may need a wake-up
    bool is_popping_false = false;
    if (!atomic_compare_exchange_strong_explicit(&self->is_popping, &is_popping_false, true, memory_order_relaxed, memory_order_relaxed)) {
        fputs("panic: htMpsc_pop: more than one threads are reading\n", stderr);
        mtx_unlock(&self->mtx_read);
        abort();
    }
    uint_fast8_t flag = atomic_load_explicit(&row->flag, memory_order_acquire);
    // Test if the front element is ready.
    while (flag == HT_MPSC_EMPTY) {
        // Wait until the front element is ready.
        if (cnd_wait(&self->cnd_read, &self->mtx_read) != thrd_success) {
            fputs("panic: htMpsc_pop: failed to wait on the conditional variable\n", stderr);
            mtx_unlock(&self->mtx_read);
            abort();
        }
        flag = atomic_load_explicit(&row->flag, memory_order_acquire);
    }
    atomic_store_explicit(&self->is_popping, false, memory_order_relaxed);
    if (mtx_unlock(&self->mtx_read) != thrd_success) {
        fputs("panic: htMpsc_pop: failed to unlock the mutex\n", stderr);
        abort();
    }

    if (flag == HT_MPSC_VALUE) {
        // Copy the data
        *time = row->time;
        memcpy(columns, row->columns, htMpsc_num_str_columns(self) * sizeof row->columns[0]);
    }
    atomic_store_explicit(&row->flag, HT_MPSC_EMPTY, memory_order_relaxed);
    // Release an index to front
    self->front++;
    if (self->front == self->cap) {
        self->front = 0;
    }

    if (mtx_lock(&self->mtx_write) != thrd_success) {
        fputs("panic: htMpsc_pop: failed to lock the mutex\n", stderr);
        abort();
    }
    // Release a slot
    atomic_fetch_sub_explicit(&self->size, 1, memory_order_release);
    // Wake up the writing thread
    if (cnd_signal(&self->cnd_write) != thrd_success) {
        fputs("panic: htMpsc_pop: failed to signal the conditional variable\n", stderr);
        abort();
    }
    if (mtx_unlock(&self->mtx_write) != thrd_success) {
        fputs("panic: htMpsc_pop: failed to unlock the mutex\n", stderr);
        abort();
    }

    return flag == HT_MPSC_VALUE;
}

void htMpsc_close(struct htMpsc *self) {
    struct htMpscRow *row = htMpsc_before_push(self);
    uint_fast8_t flag_invalid = HT_MPSC_EMPTY;
    if (!atomic_compare_exchange_strong_explicit(&row->flag, &flag_invalid, HT_MPSC_EOF, memory_order_release, memory_order_relaxed)) {
        fputs("panic: htMpsc_push: memory corruption\n", stderr);
        abort();
    }
    htMpsc_after_push(self);
}
