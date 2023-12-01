#include "csvreader.h"
#include "csvread_error.h"
#include "strbuilder.h"
#include "string.h"
#include "strview.h"
#include <errno.h>
#include <hypetrace.h>
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

static struct HTCsvReadError HTCsvReader_read_header(struct HTCsvReader *self);

struct HTCsvReadError HTCsvReader_new(struct HTCsvReader **out, FILE *file) {
    *out = malloc(sizeof **out);
    if (!*out) {
        abort();
    }

    struct HTCsvReader *self = *out;
    self->file = file;
    self->pos_row = 0;
    self->pos_col = 0;
    struct HTCsvReadError err = HTCsvReader_read_header(self);
    if (err.code != HTNoError) {
        free(self);
        return err;
    }

    size_t num_columns = HTCsvReader_num_columns(self);
    self->column_index = HTHashmap_new(num_columns);
    for (size_t i = 0; i < num_columns; i++) {
        const struct HTString *column = &self->head_buffer.data[i];
        size_t old_index = HTHashmap_try_set(&self->column_index, column->buf, column->len, i);
        if (old_index != i && column->len != 0) {
            struct HTCsvReadError err = HTCsvReadError_new_column(column->buf, column->len, old_index, i);
            HTArray_free(&self->head_buffer);
            HTHashmap_free(&self->column_index);
            free(self);
            return err;
        }
    }
    self->line_buffer = HTArray_with_capacity(num_columns);
    return HTCsvReadError_new_no_error();
}

void HTCsvReader_free(struct HTCsvReader *self) {
    fclose(self->file);
    HTArray_free(&self->head_buffer);
    HTArray_free(&self->line_buffer);
    HTHashmap_free(&self->column_index);
    free(self);
}

static void HTCsvReader_push_pending_chars(struct HTStrBuilder *field, enum State *state) {
    switch (*state) {
    case StateEF:
        HTStrBuilder_push(field, '\xef');
        *state = StateField;
        break;
    case StateEFBB:
        HTStrBuilder_push(field, '\xef');
        HTStrBuilder_push(field, '\xbb');
        *state = StateField;
        break;
    case State0D:
        HTStrBuilder_push(field, '\r');
        *state = StateField;
        break;
    default:;
    }
}

static struct HTCsvReadError HTCsvReader_read_header(struct HTCsvReader *self) {
    enum State state = StateStart;
    self->head_buffer = HTArray_new();
    struct HTStrBuilder field = HTStrBuilder_new();
    for (;;) {
        int ch = getc_unlocked(self->file);
        if (ch == EOF) {
            if (ferror_unlocked(self->file)) {
                struct HTCsvReadError err = HTCsvReadError_new_io(self->pos_row, self->pos_col, errno);
                HTStrBuilder_free(&field);
                HTArray_free(&self->head_buffer);
                return err;
            } else {
                HTCsvReader_push_pending_chars(&field, &state);
                if (state == StateStart) {
                    return HTCsvReadError_new_no_error();
                }
                if (state == StateQuote) {
                    HTStrBuilder_free(&field);
                    HTArray_free(&self->head_buffer);
                    return HTCsvReadError_new_quote(self->pos_row, self->pos_col);
                }
                HTArray_push(&self->head_buffer, field.buf, field.len, field.cap);
                HTArray_shrink(&self->head_buffer);
                return HTCsvReadError_new_no_error();
            }
        }
        switch ((char) ch) {
        case '\n':
            if (state != State0D) {
                HTCsvReader_push_pending_chars(&field, &state);
            }
            if (state == StateQuote) {
                HTStrBuilder_push(&field, (char) ch);
                break;
            }
            HTArray_push(&self->head_buffer, field.buf, field.len, field.cap);
            HTArray_shrink(&self->head_buffer);
            self->pos_row++;
            self->pos_col = 0;
            return HTCsvReadError_new_no_error();
        case '\r':
            HTCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                HTStrBuilder_push(&field, (char) ch);
                break;
            }
            state = State0D;
            break;
        case '"':
            HTCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                state = StateQuoteEnd;
                break;
            }
            if (state == StateQuoteEnd) {
                HTStrBuilder_push(&field, (char) ch);
            }
            state = StateQuote;
            break;
        case ',':
            HTCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                HTStrBuilder_push(&field, (char) ch);
                break;
            }
            HTArray_push(&self->head_buffer, field.buf, field.len, field.cap);
            field = HTStrBuilder_new();
            state = StateField;
            break;
        case '\xbb':
            if (state == StateEF) {
                state = StateEFBB;
                break;
            }
            HTCsvReader_push_pending_chars(&field, &state);
            HTStrBuilder_push(&field, (char) ch);
            break;
        case '\xbf':
            if (state == StateEFBB) {
                state = StateField;
                break;
            }
            HTCsvReader_push_pending_chars(&field, &state);
            HTStrBuilder_push(&field, (char) ch);
            break;
        case '\xef':
            HTCsvReader_push_pending_chars(&field, &state);
            if (state == StateStart) {
                state = StateEF;
                break;
            }
            HTStrBuilder_push(&field, (char) ch);
            break;
        default:
            HTCsvReader_push_pending_chars(&field, &state);
            HTStrBuilder_push(&field, (char) ch);
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

struct HTCsvReadError HTCsvReader_read_row(struct HTCsvReader *self) {
    enum State state = StateStart;
    HTArray_clear(&self->line_buffer);
    struct HTStrBuilder field = HTStrBuilder_new();
    for (;;) {
        int ch = getc_unlocked(self->file);
        if (ch == EOF) {
            if (ferror_unlocked(self->file)) {
                struct HTCsvReadError err = HTCsvReadError_new_io(self->pos_row, self->pos_col, errno);
                HTStrBuilder_free(&field);
                HTArray_clear(&self->line_buffer);
                return err;
            } else {
                HTCsvReader_push_pending_chars(&field, &state);
                if (state == StateStart) {
                    return HTCsvReadError_new_end_of_file();
                }
                if (state == StateQuote) {
                    HTStrBuilder_free(&field);
                    HTArray_clear(&self->line_buffer);
                    return HTCsvReadError_new_quote(self->pos_row, self->pos_col);
                }
                if (!HTArray_try_push(&self->line_buffer, field.buf, field.len, field.cap)) {
                    HTStrBuilder_free(&field);
                }
                return HTCsvReadError_new_no_error();
            }
        }
        switch ((char) ch) {
        case '\n':
            if (state != State0D) {
                HTCsvReader_push_pending_chars(&field, &state);
            }
            if (state == StateQuote) {
                HTStrBuilder_push(&field, (char) ch);
                break;
            }
            if (!HTArray_try_push(&self->line_buffer, field.buf, field.len, field.cap)) {
                HTStrBuilder_free(&field);
            }
            self->pos_row++;
            self->pos_col = 0;
            return HTCsvReadError_new_no_error();
        case '\r':
            HTCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                HTStrBuilder_push(&field, (char) ch);
                break;
            }
            state = State0D;
            break;
        case '"':
            HTCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                state = StateQuoteEnd;
                break;
            }
            if (state == StateQuoteEnd) {
                HTStrBuilder_push(&field, (char) ch);
            }
            state = StateQuote;
            break;
        case ',':
            HTCsvReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                HTStrBuilder_push(&field, (char) ch);
                break;
            }
            if (!HTArray_try_push(&self->line_buffer, field.buf, field.len, field.cap)) {
                HTStrBuilder_free(&field);
            }
            field = HTStrBuilder_new();
            state = StateField;
            break;
        default:
            HTCsvReader_push_pending_chars(&field, &state);
            HTStrBuilder_push(&field, (char) ch);
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

size_t HTCsvReader_num_columns(const struct HTCsvReader *self) {
    return self->head_buffer.len;
}

struct HTStrView HTCsvReader_column_name_by_index(const struct HTCsvReader *self, size_t column) {
    return HTStrView_from_HTString(self->head_buffer.data[column].buf, self->head_buffer.data[column].len);
}

_Bool HTCsvReader_column_index_by_name(const struct HTCsvReader *self, size_t *out, const char *column, size_t column_len) {
    return HTHashmap_get(&self->column_index, out, column, column_len);
}

struct HTStrView HTCsvReader_value_by_column_index(const struct HTCsvReader *self, size_t column) {
    if (column >= self->line_buffer.len) {
        return HTStrView_new();
    }
    return HTStrView_from_HTString(self->line_buffer.data[column].buf, self->line_buffer.data[column].len);
}

_Bool HTCsvReader_value_by_column_name(const struct HTCsvReader *self, struct HTStrView *out, const char *column, size_t column_len) {
    size_t index;
    if (!HTCsvReader_column_index_by_name(self, &index, column, column_len)) {
        return false;
    }
    *out = HTCsvReader_value_by_column_index(self, index);
    return true;
}
