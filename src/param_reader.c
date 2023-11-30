#include "param_reader.h"
#include "error.h"
#include "hypetrace.h"
#include "strbuf.h"
#include "string.h"
#include <errno.h>
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

static union HTError HTParamReader_read_header(struct HTParamReader *self);

union HTError HTParamReader_new(struct HTParamReader **out, FILE *file) {
    *out = malloc(sizeof **out);
    if (!*out) {
        abort();
    }

    struct HTParamReader *self = *out;
    self->file = file;
    self->pos_row = 0;
    self->pos_col = 0;
    union HTError err = HTParamReader_read_header(self);
    if (err.code != HTNoError) {
        free(self);
        return err;
    }

    size_t num_columns = self->head_buffer.len;
    self->column_index = HTHashmap_new(num_columns);
    for (size_t i = 0; i < num_columns; i++) {
        const struct HTString *column = &self->head_buffer.data[i];
        size_t old_index = HTHashmap_try_set(&self->column_index, column->buf, column->len, i);
        if (old_index != i && column->len != 0) {
            union HTError err = HTError_new_column(column->buf, column->len, old_index, i);
            HTArray_drop(&self->head_buffer);
            HTHashmap_drop(&self->column_index);
            free(self);
            return err;
        }
    }
    self->line_buffer = HTArray_with_capacity(num_columns);
    return HTError_new_no_error();
}

void HTParamReader_drop(struct HTParamReader *self) {
    fclose(self->file);
    HTArray_drop(&self->head_buffer);
    HTArray_drop(&self->line_buffer);
    HTHashmap_drop(&self->column_index);
    free(self);
}

static void HTParamReader_push_pending_chars(struct HTStrBuf *field, enum State *state) {
    switch (*state) {
    case StateEF:
        HTStrBuf_push(field, '\xef');
        *state = StateField;
        break;
    case StateEFBB:
        HTStrBuf_push(field, '\xef');
        HTStrBuf_push(field, '\xbb');
        *state = StateField;
        break;
    case State0D:
        HTStrBuf_push(field, '\r');
        *state = StateField;
        break;
    default:;
    }
}

static union HTError HTParamReader_read_header(struct HTParamReader *self) {
    enum State state = StateStart;
    self->head_buffer = HTArray_new();
    struct HTStrBuf field = HTStrBuf_new();
    for (;;) {
        int ch = getc_unlocked(self->file);
        if (ch == EOF) {
            if (ferror_unlocked(self->file)) {
                union HTError err = HTError_new_io(self->pos_row, self->pos_col, errno);
                HTStrBuf_drop(&field);
                HTArray_drop(&self->head_buffer);
                return err;
            } else {
                HTParamReader_push_pending_chars(&field, &state);
                if (state == StateStart) {
                    return HTError_new_no_error();
                }
                if (state == StateQuote) {
                    HTStrBuf_drop(&field);
                    HTArray_drop(&self->head_buffer);
                    return HTError_new_quote(self->pos_row, self->pos_col);
                }
                HTArray_push(&self->head_buffer, field.buf, field.len, field.cap);
                return HTError_new_no_error();
            }
        }
        switch ((char) ch) {
        case '\n':
            if (state != State0D) {
                HTParamReader_push_pending_chars(&field, &state);
            }
            if (state == StateQuote) {
                HTStrBuf_push(&field, (char) ch);
                break;
            }
            HTArray_push(&self->head_buffer, field.buf, field.len, field.cap);
            self->pos_row++;
            self->pos_col = 0;
            return HTError_new_no_error();
        case '\r':
            HTParamReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                HTStrBuf_push(&field, (char) ch);
                break;
            }
            state = State0D;
            break;
        case '"':
            HTParamReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                state = StateQuoteEnd;
                break;
            }
            if (state == StateQuoteEnd) {
                HTStrBuf_push(&field, (char) ch);
            }
            state = StateQuote;
            break;
        case ',':
            HTParamReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                HTStrBuf_push(&field, (char) ch);
                break;
            }
            HTArray_push(&self->head_buffer, field.buf, field.len, field.cap);
            field = HTStrBuf_new();
            state = StateField;
            break;
        case '\xbb':
            if (state == StateEF) {
                state = StateEFBB;
                break;
            }
            HTParamReader_push_pending_chars(&field, &state);
            HTStrBuf_push(&field, (char) ch);
            break;
        case '\xbf':
            if (state == StateEFBB) {
                state = StateField;
                break;
            }
            HTParamReader_push_pending_chars(&field, &state);
            HTStrBuf_push(&field, (char) ch);
            break;
        case '\xef':
            HTParamReader_push_pending_chars(&field, &state);
            if (state == StateStart) {
                state = StateEF;
                break;
            }
            HTStrBuf_push(&field, (char) ch);
            break;
        default:
            HTParamReader_push_pending_chars(&field, &state);
            HTStrBuf_push(&field, (char) ch);
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

union HTError HTParamReader_read_row(struct HTParamReader *self) {
    enum State state = StateStart;
    HTArray_clear(&self->line_buffer);
    struct HTStrBuf field = HTStrBuf_new();
    for (;;) {
        int ch = getc_unlocked(self->file);
        if (ch == EOF) {
            if (ferror_unlocked(self->file)) {
                union HTError err = HTError_new_io(self->pos_row, self->pos_col, errno);
                HTStrBuf_drop(&field);
                HTArray_clear(&self->line_buffer);
                return err;
            } else {
                HTParamReader_push_pending_chars(&field, &state);
                if (state == StateStart) {
                    return HTError_new_end_of_file();
                }
                if (state == StateQuote) {
                    HTStrBuf_drop(&field);
                    HTArray_clear(&self->line_buffer);
                    return HTError_new_quote(self->pos_row, self->pos_col);
                }
                if (!HTArray_try_push(&self->line_buffer, field.buf, field.len, field.cap)) {
                    HTStrBuf_drop(&field);
                }
                return HTError_new_no_error();
            }
        }
        switch ((char) ch) {
        case '\n':
            if (state != State0D) {
                HTParamReader_push_pending_chars(&field, &state);
            }
            if (state == StateQuote) {
                HTStrBuf_push(&field, (char) ch);
                break;
            }
            if (!HTArray_try_push(&self->line_buffer, field.buf, field.len, field.cap)) {
                HTStrBuf_drop(&field);
            }
            self->pos_row++;
            self->pos_col = 0;
            return HTError_new_no_error();
        case '\r':
            HTParamReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                HTStrBuf_push(&field, (char) ch);
                break;
            }
            state = State0D;
            break;
        case '"':
            HTParamReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                state = StateQuoteEnd;
                break;
            }
            if (state == StateQuoteEnd) {
                HTStrBuf_push(&field, (char) ch);
            }
            state = StateQuote;
            break;
        case ',':
            HTParamReader_push_pending_chars(&field, &state);
            if (state == StateQuote) {
                HTStrBuf_push(&field, (char) ch);
                break;
            }
            if (!HTArray_try_push(&self->line_buffer, field.buf, field.len, field.cap)) {
                HTStrBuf_drop(&field);
            }
            field = HTStrBuf_new();
            state = StateField;
            break;
        default:
            HTParamReader_push_pending_chars(&field, &state);
            HTStrBuf_push(&field, (char) ch);
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

bool HTParamReader_get(struct HTParamReader *self, const char *column, size_t column_len, struct HTStrView *out) {
    size_t index;
    if (!HTHashmap_get(&self->column_index, &index, column, column_len)) {
        return false;
    }
    if (index >= self->line_buffer.len) {
        out->buf = NULL;
        out->len = 0;
    } else {
        out->buf = self->line_buffer.data[index].buf;
        out->len = self->line_buffer.data[index].len;
    }
    return true;
}
