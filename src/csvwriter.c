#include "csvwriter.h"
#include "csvwrite_error.h"
#include <errno.h>
#include <hypetrace.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HTCsvWriterValue {
    char *buf;
    size_t len;
    void (*free_func)(void *param);
    void *free_param;
    bool valid;
};

static union HTCsvWriteError HTCsvWriter_write_header(struct HTCsvWriter *self);
static union HTCsvWriteError HTCsvWriter_write_value(struct HTCsvWriter *self, const struct HTCsvWriterValue *value);

union HTCsvWriteError HTCsvWriter_new(struct HTCsvWriter **out, FILE *file, struct HTStrView *header, size_t num_columns) {
    *out = malloc(sizeof **out);
    if (!*out) {
        abort();
    }

    struct HTCsvWriter *self = *out;
    self->file = file;
    self->num_columns = num_columns;
    self->line_buffer = calloc(num_columns, sizeof self->line_buffer[0]);
    if (!self->line_buffer) {
        abort();
    }
    self->column_index = HTHashmap_new(num_columns);

    for (size_t i = 0; i < num_columns; i++) {
        self->line_buffer[i].buf = (char *) header[i].buf;
        self->line_buffer[i].len = header[i].len;
        self->line_buffer[i].valid = true;
    }
    for (size_t i = 0; i < num_columns; i++) {
        if (HTHashmap_try_set(&self->column_index, header[i].buf, header[i].len, i) != i) {
            fputs("panic: HTCsvWriter_new: duplicate column names\n", stderr);
            abort();
        };
    }

    union HTCsvWriteError err = HTCsvWriter_write_header(self);
    if (err.code != HTNoError) {
        free(self->line_buffer);
        HTHashmap_free(&self->column_index);
        free(self);
        return err;
    }

    return HTCsvWriteError_new_no_error();
}

static void HTCsvWriter_free_line_buffer(struct HTCsvWriter *self) {
    for (size_t i = 0; i < self->num_columns; i++) {
        if (self->line_buffer[i].valid && self->line_buffer[i].free_func) {
            self->line_buffer[i].free_func(self->line_buffer[i].free_param);
        }
    }
    memset(self->line_buffer, 0, self->num_columns * sizeof self->line_buffer[0]);
}

void HTCsvWriter_free(struct HTCsvWriter *self) {
    HTCsvWriter_free_line_buffer(self);
    free(self->line_buffer);
    HTHashmap_free(&self->column_index);
    free(self);
}

static union HTCsvWriteError HTCsvWriter_write_header(struct HTCsvWriter *self) {
    const char utf8_mark[3] = {'\xef', '\xbb', '\xbf'};
    for (size_t i = 0; i < 3; i++) {
        if (putc_unlocked(utf8_mark[i], self->file) == EOF) {
            return HTCsvWriteError_new_io(errno);
        }
    }
    return HTCsvWriter_write_row(self);
}

union HTCsvWriteError HTCsvWriter_write_row(struct HTCsvWriter *self) {
    for (size_t i = 0; i < self->num_columns; i++) {
        if (!self->line_buffer[i].valid) {
            fprintf(stderr, "panic: HTCsvWriter_write_row: column #%zu is not set\n", i);
            abort();
        }
    }
    for (size_t i = 0; i < self->num_columns; i++) {
        if (i != 0) {
            if (putc_unlocked(',', self->file) == EOF) {
                HTCsvWriter_free_line_buffer(self);
                return HTCsvWriteError_new_io(errno);
            }
        }
        union HTCsvWriteError err = HTCsvWriter_write_value(self, &self->line_buffer[i]);
        if (err.code != HTNoError) {
            HTCsvWriter_free_line_buffer(self);
            return err;
        }
    }
    const char crlf[2] = {'\r', '\n'};
    for (size_t i = 0; i < 2; i++) {
        if (putc_unlocked(crlf[i], self->file) == EOF) {
            HTCsvWriter_free_line_buffer(self);
            return HTCsvWriteError_new_io(errno);
        }
    }
    if (fflush(self->file) == EOF) {
        HTCsvWriter_free_line_buffer(self);
        return HTCsvWriteError_new_io(errno);
    }
    HTCsvWriter_free_line_buffer(self);
    return HTCsvWriteError_new_no_error();
}

static union HTCsvWriteError HTCsvWriter_write_value(struct HTCsvWriter *self, const struct HTCsvWriterValue *value) {
    bool need_escaping = false;
    for (size_t i = 0; i < value->len; i++) {
        if (value->buf[i] == '\n' || value->buf[i] == '\r' || value->buf[i] == '"' || value->buf[i] == ',') {
            need_escaping = true;
        }
    }
    if (need_escaping) {
        if (putc_unlocked('"', self->file) == EOF) {
            return HTCsvWriteError_new_io(errno);
        }
    }
    for (size_t i = 0; i < value->len; i++) {
        if (putc_unlocked(value->buf[i], self->file) == EOF) {
            return HTCsvWriteError_new_io(errno);
        }
        if (value->buf[i] == '"') {
            if (putc_unlocked('"', self->file) == EOF) {
                return HTCsvWriteError_new_io(errno);
            }
        }
    }
    if (need_escaping) {
        if (putc_unlocked('"', self->file) == EOF) {
            return HTCsvWriteError_new_io(errno);
        }
    }
    return HTCsvWriteError_new_no_error();
}

void HTCsvWriter_set_string_by_column_index(struct HTCsvWriter *self, size_t column, char *value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param) {
    if (self->line_buffer[column].valid) {
        fprintf(stderr, "panic: HTCsvWriter_set_string_by_column_index: duplicate column #%zu\n", column);
        abort();
    }
    self->line_buffer[column].buf = value;
    self->line_buffer[column].len = value_len;
    self->line_buffer[column].free_func = value_free_func;
    self->line_buffer[column].free_param = value_free_param;
    self->line_buffer[column].valid = true;
}

void HTCsvWriter_set_strview_by_column_index(struct HTCsvWriter *self, size_t column, const char *value, size_t value_len) {
    if (self->line_buffer[column].valid) {
        fprintf(stderr, "panic: HTCsvWriter_set_strview_by_column_index: duplicate column #%zu\n", column);
        abort();
    }
    self->line_buffer[column].buf = (char *) value;
    self->line_buffer[column].len = value_len;
    self->line_buffer[column].valid = true;
}

static size_t HTCsvWriter_column_index_by_name(const struct HTCsvWriter *self, const char *column, size_t column_len) {
    size_t out;
    if (!HTHashmap_get(&self->column_index, &out, column, column_len)) {
        fputs("panic: HTCsvWriter_set_string_by_column_index: unknown column ", stderr);
        fwrite(column, 1, column_len, stderr);
        putc('\n', stderr);
        abort();
    }
    return out;
}

void HTCsvWriter_set_string_by_column_name(struct HTCsvWriter *self, const char *HT_restrict column, size_t column_len, char *value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param) {
    HTCsvWriter_set_string_by_column_index(self, HTCsvWriter_column_index_by_name(self, column, column_len), value, value_len, value_free_func, value_free_param);
}

void HTCsvWriter_set_strview_by_column_name(struct HTCsvWriter *self, const char *HT_restrict column, size_t column_len, const char *HT_restrict value, size_t value_len) {
    HTCsvWriter_set_strview_by_column_index(self, HTCsvWriter_column_index_by_name(self, column, column_len), value, value_len);
}
