#define _GNU_SOURCE

#include "csvwriter.h"
#include "csvwrite_error.h"
#include "string.h"
#include <errno.h>
#include <hypertracer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__) && !defined(fflush_unlocked)
#define fflush_unlocked fflush
#endif

struct htCsvWriterValue {
    struct htString str;
    bool valid;
};

static union htCsvWriteError htCsvWriter_write_header(struct htCsvWriter *self);
static union htCsvWriteError htCsvWriter_write_value(struct htCsvWriter *self, const struct htCsvWriterValue *value);

union htCsvWriteError htCsvWriter_new(struct htCsvWriter **out, FILE *file, const struct htStrView header[], size_t num_columns) {
    *out = malloc(sizeof **out);
    if (!*out) {
        abort();
    }

    struct htCsvWriter *self = *out;
    self->file = file;
    self->num_columns = num_columns;
    self->line_buffer = calloc(num_columns, sizeof self->line_buffer[0]);
    if (!self->line_buffer) {
        free(self);
        abort();
    }
    self->column_index = htHashmap_new(num_columns);

    for (size_t i = 0; i < num_columns; i++) {
        self->line_buffer[i].str.buf = (char *) header[i].buf;
        self->line_buffer[i].str.len = header[i].len;
        self->line_buffer[i].valid = true;
    }
    for (size_t i = 0; i < num_columns; i++) {
        if (htHashmap_try_set(&self->column_index, header[i].buf, header[i].len, i) != i) {
            fputs("panic: htCsvWriter_new: duplicate column names\n", stderr);
            free(self->line_buffer);
            htHashmap_free(&self->column_index);
            free(self);
            abort();
        };
    }

    union htCsvWriteError err = htCsvWriter_write_header(self);
    if (err.code != HTNoError) {
        free(self->line_buffer);
        htHashmap_free(&self->column_index);
        free(self);
        return err;
    }

    return htCsvWriteError_new_no_error();
}

static void htCsvWriter_free_line_buffer(struct htCsvWriter *self) {
    for (size_t i = 0; i < self->num_columns; i++) {
        if (self->line_buffer[i].valid) {
            htString_free(&self->line_buffer[i].str);
        }
    }
    memset(self->line_buffer, 0, self->num_columns * sizeof self->line_buffer[0]);
}

void htCsvWriter_free(struct htCsvWriter *self) {
    htCsvWriter_free_line_buffer(self);
    free(self->line_buffer);
    htHashmap_free(&self->column_index);
    free(self);
}

static union htCsvWriteError htCsvWriter_write_header(struct htCsvWriter *self) {
    const char utf8_mark[3] = {'\xef', '\xbb', '\xbf'};
    for (size_t i = 0; i < 3; i++) {
        if (putc_unlocked(utf8_mark[i], self->file) == EOF) {
            return htCsvWriteError_new_io(errno);
        }
    }
    return htCsvWriter_write_row(self);
}

union htCsvWriteError htCsvWriter_write_row(struct htCsvWriter *self) {
    for (size_t i = 0; i < self->num_columns; i++) {
        if (!self->line_buffer[i].valid) {
            fprintf(stderr, "panic: htCsvWriter_write_row: column #%zu is not set\n", i);
            abort();
        }
    }
    for (size_t i = 0; i < self->num_columns; i++) {
        if (i != 0) {
            if (putc_unlocked(',', self->file) == EOF) {
                htCsvWriter_free_line_buffer(self);
                return htCsvWriteError_new_io(errno);
            }
        }
        union htCsvWriteError err = htCsvWriter_write_value(self, &self->line_buffer[i]);
        if (err.code != HTNoError) {
            htCsvWriter_free_line_buffer(self);
            return err;
        }
    }
    const char crlf[2] = {'\r', '\n'};
    for (size_t i = 0; i < 2; i++) {
        if (putc_unlocked(crlf[i], self->file) == EOF) {
            htCsvWriter_free_line_buffer(self);
            return htCsvWriteError_new_io(errno);
        }
    }
    if (fflush_unlocked(self->file) == EOF) {
        htCsvWriter_free_line_buffer(self);
        return htCsvWriteError_new_io(errno);
    }
    htCsvWriter_free_line_buffer(self);
    return htCsvWriteError_new_no_error();
}

static union htCsvWriteError htCsvWriter_write_value(struct htCsvWriter *self, const struct htCsvWriterValue *value) {
    bool need_escaping = false;
    for (size_t i = 0; i < value->str.len; i++) {
        if (value->str.buf[i] == '\n' || value->str.buf[i] == '\r' || value->str.buf[i] == '"' || value->str.buf[i] == ',') {
            need_escaping = true;
        }
    }
    if (need_escaping) {
        if (putc_unlocked('"', self->file) == EOF) {
            return htCsvWriteError_new_io(errno);
        }
    }
    for (size_t i = 0; i < value->str.len; i++) {
        if (putc_unlocked(value->str.buf[i], self->file) == EOF) {
            return htCsvWriteError_new_io(errno);
        }
        if (value->str.buf[i] == '"') {
            if (putc_unlocked('"', self->file) == EOF) {
                return htCsvWriteError_new_io(errno);
            }
        }
    }
    if (need_escaping) {
        if (putc_unlocked('"', self->file) == EOF) {
            return htCsvWriteError_new_io(errno);
        }
    }
    return htCsvWriteError_new_no_error();
}

void htCsvWriter_set_string_by_column_index(struct htCsvWriter *self, size_t column, char *value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param) {
    if (self->line_buffer[column].valid) {
        fprintf(stderr, "panic: htCsvWriter_set_string_by_column_index: duplicate column #%zu\n", column);
        abort();
    }
    self->line_buffer[column].str.buf = value;
    self->line_buffer[column].str.len = value_len;
    self->line_buffer[column].str.free_func = value_free_func;
    self->line_buffer[column].str.free_param = value_free_param;
    self->line_buffer[column].valid = true;
}

void htCsvWriter_set_strview_by_column_index(struct htCsvWriter *self, size_t column, const char *value, size_t value_len) {
    if (self->line_buffer[column].valid) {
        fprintf(stderr, "panic: htCsvWriter_set_strview_by_column_index: duplicate column #%zu\n", column);
        abort();
    }
    self->line_buffer[column].str.buf = (char *) value;
    self->line_buffer[column].str.len = value_len;
    self->line_buffer[column].valid = true;
}

static size_t htCsvWriter_column_index_by_name(const struct htCsvWriter *self, const char *column, size_t column_len) {
    size_t out;
    if (!htHashmap_get(&self->column_index, &out, column, column_len)) {
        fputs("panic: htCsvWriter_set_string_by_column_index: unknown column ", stderr);
        fwrite(column, 1, column_len, stderr);
        putc('\n', stderr);
        abort();
    }
    return out;
}

void htCsvWriter_set_string_by_column_name(struct htCsvWriter *self, const char *HT_RESTRICT column, size_t column_len, char *value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param) {
    htCsvWriter_set_string_by_column_index(self, htCsvWriter_column_index_by_name(self, column, column_len), value, value_len, value_free_func, value_free_param);
}

void htCsvWriter_set_strview_by_column_name(struct htCsvWriter *self, const char *HT_RESTRICT column, size_t column_len, const char *HT_RESTRICT value, size_t value_len) {
    htCsvWriter_set_strview_by_column_index(self, htCsvWriter_column_index_by_name(self, column, column_len), value, value_len);
}
