#include "tracer.h"
#include "csvwrite_error.h"
#include "mpsc.h"
#include "strview.h"
#include <hypertracer.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

static int HTTracer_thread_start(void *param) {
    struct HTTracer *self = param;
    size_t num_str_columns = HTMpsc_num_str_columns(&self->chan);
    struct timespec time;
    struct HTString *columns = malloc(num_str_columns * sizeof columns[0]);
    uint64_t line_number = 0;

    while (HTMpsc_pop(&self->chan, &time, columns)) {
        line_number++;
        char line_number_str[21]; // "18446744073709551615\0"
        int line_number_len = snprintf(line_number_str, sizeof line_number_str, "%" PRIu64, line_number);
        if (line_number_len < 0 || line_number_len >= sizeof line_number_str) {
            fputs("panic: HTTracer_thread_start: failed to format line number\n", stderr);
            free(columns);
            abort();
        }
        HTCsvWriter_set_strview_by_column_index(&self->csv_writer, 0, line_number_str, line_number_len);

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
        if (time_len < 0 || time_len >= sizeof time_str) {
            fputs("panic: HTTracer_thread_start: failed to format time\n", stderr);
            free(columns);
            abort();
        }
        HTCsvWriter_set_strview_by_column_index(&self->csv_writer, 1, time_str, time_len);

        for (size_t i = 0; i < num_str_columns; i++) {
            HTCsvWriter_set_string_by_column_index(&self->csv_writer, i + 2, columns[i].buf, columns[i].len, columns[i].free_func, columns[i].free_param);
        }

        HTCsvWriteError err = HTCsvWriter_write_row(&self->csv_writer);
        if (err.code != HTNoError) {
            self->err = err;
            while (HTMpsc_pop(&self->chan, &time, columns)) {
            }
            break;
        }
    }

    free(columns);
    return 0;
}

union HTCsvWriteError HTTracer_new(struct HTTracer **out, FILE *file, size_t buffer_num_rows, const struct HTStrView header[], size_t num_str_columns) {
    if (num_str_columns > SIZE_MAX - 2) {
        fprintf(stderr, "panic: HTTracer_new: too many columns %zu\n", num_str_columns);
        abort();
    }
    struct HTStrView *header_with_time = malloc((num_str_columns + 2) * sizeof header_with_time[0]);
    if (!header_with_time) {
        abort();
    }
    header_with_time[0] = HTStrView_empty();
    static const char str_time[4] = {'t', 'i', 'm', 'e'};
    header_with_time[1] = HTStrView_new(str_time, 4);
    memcpy(&header_with_time[2], header, num_str_columns * sizeof header[0]);

    *out = malloc(sizeof **out);
    if (!*out) {
        abort();
    }

    struct HTTracer *self = *out;
    HTCsvWriteError err = HTCsvWriter_new(&self->csv_writer, file, header_with_time, num_str_columns + 2);
    free(header_with_time);
    if (err.code != HTNoError) {
        HTCsvWriter_free(&self->csv_writer);
        free(self);
        return err;
    }
    HTMpsc_new(&self->chan, buffer_num_rows, num_str_columns);
    self->err = HTCsvWriteError_new_no_error();
    if (thrd_create(&self->write_thread, HTTracer_thread_start, self) != thrd_success) {
        fputs("panic: HTTracer_new: failed to create the writing thread\n", stderr);
        HTMpsc_free(&self->chan);
        HTCsvWriter_free(&self->csv_writer);
        free(self);
        abort();
    }
    return HTCsvWriteError_new_no_error();
}

union HTCsvWriteError HTTracer_free(struct HTTracer *self) {
    HTMpsc_close(&self->chan);
    int result;
    if (thrd_join(self->write_thread, &result) != thrd_success || result != 0) {
        fputs("panic: HTTracer_free: failed to join the writing thread\n", stderr);
        abort();
    }
    HTCsvWriter_free(&self->csv_writer);
    return self->err;
}

void HTTracer_write_row(struct HTTracer *self, struct HTString *columns[]) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    HTMpsc_push(&self->chan, &time, columns);
}
