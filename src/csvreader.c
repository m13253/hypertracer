#define _GNU_SOURCE

#include "csvreader.h"
#include "csvread_error.h"
#include "strbuilder.h"
#include "strview.h"
#include <errno.h>
#include <hypertracer.h>
#include <stdbool.h>
#include <stdlib.h>

enum State {
    StateStart,
    StateEF,
    StateEFBB,
    StateField,
    StateQuote,
    StateQuoteEnd,
    State0D,
};

static union htCsvReadError htCsvReader_read_header(struct htCsvReader *self);

union htCsvReadError htCsvReader_new(struct htCsvReader **out, FILE *file) {
    *out = malloc(sizeof **out);
    if (!*out) {
        abort();
    }

    struct htCsvReader *self = *out;
    self->file = file;
    self->pos_row = 0;
    self->pos_col = 0;
    union htCsvReadError err = htCsvReader_read_header(self);
    if (err.code != HTNoError) {
        free(self);
        return err;
    }

    size_t num_columns = htCsvReader_num_columns(self);
    self->column_index = htHashmap_new(num_columns);
    for (size_t i = 0; i < num_columns; i++) {
        const struct htStrView *column = &self->head_buffer.data[i];
        size_t old_index = htHashmap_try_set(&self->column_index, column->buf, column->len, i);
        if (old_index != i && column->len != 0) {
            union htCsvReadError err = htCsvReadError_new_column(column->buf, column->len, old_index, i);
            htArray_free(&self->head_buffer);
            htHashmap_free(&self->column_index);
            free(self);
            return err;
        }
    }
    self->line_buffer = htArray_with_capacity(num_columns);
    return htCsvReadError_new_no_error();
}

void htCsvReader_free(struct htCsvReader *self) {
    htArray_free(&self->head_buffer);
    htArray_free(&self->line_buffer);
    htHashmap_free(&self->column_index);
    free(self);
}

static void htCsvReader_push_pending_chars(struct htStrBuilder *field, enum State *state) {
    switch (*state) {
    case StateEF:
        htStrBuilder_push(field, '\xef');
        *state = StateField;
        break;
    case StateEFBB:
        htStrBuilder_push(field, '\xef');
        htStrBuilder_push(field, '\xbb');
        *state = StateField;
        break;
    case State0D:
        htStrBuilder_push(field, '\r');
        *state = StateField;
        break;
    default:;
    }
}

static union htCsvReadError htCsvReader_read_header(struct htCsvReader *self) {
    enum State state = StateStart;
    self->head_buffer = htArray_new();
    struct htStrBuilder field = htStrBuilder_new();
    for (;;) {
        int ch = getc_unlocked(self->file);
        if (ch == EOF) {
            if (ferror_unlocked(self->file)) {
                union htCsvReadError err = htCsvReadError_new_io(self->pos_row, self->pos_col, errno);
                htStrBuilder_free(&field);
                htArray_free(&self->head_buffer);
                return err;
            } else {
                htCsvReader_push_pending_chars(&field, &state);
                if (state == StateStart) {
                    return htCsvReadError_new_no_error();
                }
                if (state == StateQuote) {
                    htStrBuilder_free(&field);
                    htArray_free(&self->head_buffer);
                    return htCsvReadError_new_quote(self->pos_row, self->pos_col);
                }
                htArray_push(&self->head_buffer, &field);
                htArray_shrink(&self->head_buffer);
                return htCsvReadError_new_no_error();
            }
        }
        switch ((char) ch) {
        case '\n':
            if (state != State0D) {
                htCsvReader_push_pending_chars(&field, &state);
            }
            if (state == StateQuote) {
                htStrBuilder_push(&field, (char) ch);
                break;
            }
            htArray_push(&self->head_buffer, &field);
            htArray_shrink(&self->head_buffer);
            self->pos_row++;
            self->pos_col = 0;
            return htCsvReadError_new_no_error();
        case '\r':
            htCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                htStrBuilder_push(&field, (char) ch);
                break;
            }
            state = State0D;
            break;
        case '"':
            htCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                state = StateQuoteEnd;
                break;
            }
            if (state == StateQuoteEnd) {
                htStrBuilder_push(&field, (char) ch);
            }
            state = StateQuote;
            break;
        case ',':
            htCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                htStrBuilder_push(&field, (char) ch);
                break;
            }
            htArray_push(&self->head_buffer, &field);
            field = htStrBuilder_new();
            state = StateField;
            break;
        case '\xbb':
            if (state == StateEF) {
                state = StateEFBB;
                break;
            }
            htCsvReader_push_pending_chars(&field, &state);
            htStrBuilder_push(&field, (char) ch);
            break;
        case '\xbf':
            if (state == StateEFBB) {
                state = StateField;
                break;
            }
            htCsvReader_push_pending_chars(&field, &state);
            htStrBuilder_push(&field, (char) ch);
            break;
        case '\xef':
            htCsvReader_push_pending_chars(&field, &state);
            if (state == StateStart) {
                state = StateEF;
                break;
            }
            htStrBuilder_push(&field, (char) ch);
            break;
        default:
            htCsvReader_push_pending_chars(&field, &state);
            htStrBuilder_push(&field, (char) ch);
            if (state != StateQuote) {
                state = StateField;
            }
        }
        if (ch == '\n') {
            self->pos_row++;
            self->pos_col = 0;
        } else {
            self->pos_col++;
        }
    }
}

union htCsvReadError htCsvReader_read_row(struct htCsvReader *self, HT_BOOL *eof) {
    enum State state = StateStart;
    htArray_clear(&self->line_buffer);
    struct htStrBuilder field = htStrBuilder_new();
    for (;;) {
        int ch = getc_unlocked(self->file);
        if (ch == EOF) {
            if (ferror_unlocked(self->file)) {
                union htCsvReadError err = htCsvReadError_new_io(self->pos_row, self->pos_col, errno);
                htStrBuilder_free(&field);
                htArray_clear(&self->line_buffer);
                *eof = false;
                return err;
            } else {
                htCsvReader_push_pending_chars(&field, &state);
                if (state == StateStart) {
                    *eof = true;
                    return htCsvReadError_new_no_error();
                }
                if (state == StateQuote) {
                    htStrBuilder_free(&field);
                    htArray_clear(&self->line_buffer);
                    *eof = false;
                    return htCsvReadError_new_quote(self->pos_row, self->pos_col);
                }
                if (!htArray_try_push(&self->line_buffer, &field)) {
                    htStrBuilder_free(&field);
                }
                *eof = false;
                return htCsvReadError_new_no_error();
            }
        }
        switch ((char) ch) {
        case '\n':
            if (state != State0D) {
                htCsvReader_push_pending_chars(&field, &state);
            }
            if (state == StateQuote) {
                htStrBuilder_push(&field, (char) ch);
                break;
            }
            if (!htArray_try_push(&self->line_buffer, &field)) {
                htStrBuilder_free(&field);
            }
            self->pos_row++;
            self->pos_col = 0;
            *eof = false;
            return htCsvReadError_new_no_error();
        case '\r':
            htCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                htStrBuilder_push(&field, (char) ch);
                break;
            }
            state = State0D;
            break;
        case '"':
            htCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                state = StateQuoteEnd;
                break;
            }
            if (state == StateQuoteEnd) {
                htStrBuilder_push(&field, (char) ch);
            }
            state = StateQuote;
            break;
        case ',':
            htCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                htStrBuilder_push(&field, (char) ch);
                break;
            }
            if (!htArray_try_push(&self->line_buffer, &field)) {
                htStrBuilder_free(&field);
            }
            field = htStrBuilder_new();
            state = StateField;
            break;
        default:
            htCsvReader_push_pending_chars(&field, &state);
            htStrBuilder_push(&field, (char) ch);
            if (state != StateQuote) {
                state = StateField;
            }
        }
        if (ch == '\n') {
            self->pos_row++;
            self->pos_col = 0;
        } else {
            self->pos_col++;
        }
    }
}

size_t htCsvReader_num_columns(const struct htCsvReader *self) {
    return self->head_buffer.len;
}

struct htStrView htCsvReader_column_name_by_index(const struct htCsvReader *self, size_t column) {
    return htStrView_new(self->head_buffer.data[column].buf, self->head_buffer.data[column].len);
}

HT_BOOL htCsvReader_column_index_by_name(const struct htCsvReader *self, size_t *out, const char *column, size_t column_len) {
    return htHashmap_get(&self->column_index, out, column, column_len);
}

struct htStrView htCsvReader_value_by_column_index(const struct htCsvReader *self, size_t column) {
    if (column >= self->line_buffer.len) {
        return htStrView_empty();
    }
    return htStrView_new(self->line_buffer.data[column].buf, self->line_buffer.data[column].len);
}

HT_BOOL htCsvReader_value_by_column_name(const struct htCsvReader *self, struct htStrView *out, const char *column, size_t column_len) {
    size_t index;
    if (!htCsvReader_column_index_by_name(self, &index, column, column_len)) {
        return false;
    }
    *out = htCsvReader_value_by_column_index(self, index);
    return true;
}
