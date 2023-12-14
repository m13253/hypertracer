#define _GNU_SOURCE

#include "tracer.h"
#include "csvwrite_error.h"
#include "mpsc.h"
#include "string.h"
#include "strview.h"
#include <hypertracer.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

static int htTracer_thread_start(void *param) {
    struct htTracer *self = param;
    size_t num_str_columns = htMpsc_num_str_columns(&self->chan);
    struct timespec time;
    struct htString *columns = malloc(num_str_columns * sizeof columns[0]);
    uint64_t line_number = 0;

    while (htMpsc_pop(&self->chan, &time, columns)) {
        line_number++;
        char line_number_str[21]; // "18446744073709551615\0"
        int line_number_len = snprintf(line_number_str, sizeof line_number_str, "%" PRIu64, line_number);
        if (line_number_len < 0 || line_number_len >= (int) sizeof line_number_str) {
            fputs("panic: htTracer_thread_start: failed to format line number\n", stderr);
            free(columns);
            abort();
        }
        htCsvWriter_set_strview_by_column_index(self->csv_writer, 0, line_number_str, line_number_len);

        int64_t time_sec, time_nsec;
        if (time.tv_sec < 0 && time.tv_nsec != 0) {
            time_sec = time.tv_sec + 1;
            time_nsec = 1000000000 - time.tv_nsec;
        } else {
            time_sec = time.tv_sec;
            time_nsec = time.tv_nsec;
        }
        char time_str[42]; // "-9223372036854775808.-9223372036854775808\0"
        int time_len = snprintf(time_str, sizeof time_str, "%" PRId64 ".%09" PRId64, time_sec, time_nsec);
        if (time_len < 0 || time_len >= (int) sizeof time_str) {
            fputs("panic: htTracer_thread_start: failed to format time\n", stderr);
            free(columns);
            abort();
        }
        htCsvWriter_set_strview_by_column_index(self->csv_writer, 1, time_str, time_len);

        for (size_t i = 0; i < num_str_columns; i++) {
            htCsvWriter_set_string_by_column_index(self->csv_writer, i + 2, columns[i].buf, columns[i].len, columns[i].free_func, columns[i].free_param);
        }

        htCsvWriteError err = htCsvWriter_write_row(self->csv_writer);
        if (err.code != HTNoError) {
            self->err = err;
            while (htMpsc_pop(&self->chan, &time, columns)) {
                for (size_t i = 0; i < num_str_columns; i++) {
                    htString_free(&columns[i]);
                }
            }
            break;
        }
    }

    free(columns);
    return 0;
}

union htCsvWriteError htTracer_new(struct htTracer **out, FILE *file, const struct htStrView header[], size_t num_str_columns, size_t buffer_num_rows) {
    if (num_str_columns > SIZE_MAX - 2) {
        fprintf(stderr, "panic: htTracer_new: too many columns %zu\n", num_str_columns);
        abort();
    }
    struct htStrView *header_with_time = malloc((num_str_columns + 2) * sizeof header_with_time[0]);
    if (!header_with_time) {
        abort();
    }
    header_with_time[0] = htStrView_empty();
    static const char str_time[4] = {'t', 'i', 'm', 'e'};
    header_with_time[1] = htStrView_new(str_time, 4);
    memcpy(&header_with_time[2], header, num_str_columns * sizeof header[0]);

    *out = malloc(sizeof **out);
    if (!*out) {
        abort();
    }

    struct htTracer *self = *out;
    htCsvWriteError err = htCsvWriter_new(&self->csv_writer, file, header_with_time, num_str_columns + 2);
    free(header_with_time);
    if (err.code != HTNoError) {
        htCsvWriter_free(self->csv_writer);
        free(self);
        return err;
    }
    htMpsc_new(&self->chan, buffer_num_rows, num_str_columns);
    self->err = htCsvWriteError_new_no_error();
    if (thrd_create(&self->write_thread, htTracer_thread_start, self) != thrd_success) {
        fputs("panic: htTracer_new: failed to create the writing thread\n", stderr);
        htMpsc_free(&self->chan);
        htCsvWriter_free(self->csv_writer);
        free(self);
        abort();
    }
    return htCsvWriteError_new_no_error();
}

union htCsvWriteError htTracer_free(struct htTracer *self) {
    htMpsc_close(&self->chan);
    int result;
    if (thrd_join(self->write_thread, &result) != thrd_success || result != 0) {
        fputs("panic: htTracer_free: failed to join the writing thread\n", stderr);
        abort();
    }
    htMpsc_free(&self->chan);
    htCsvWriter_free(self->csv_writer);
    union htCsvWriteError err = self->err;
    free(self);
    return err;
}

void htTracer_write_row(struct htTracer *self, struct htString columns[]) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    htMpsc_push(&self->chan, &time, columns);
}
