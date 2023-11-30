#ifndef HT_ERROR_H
#define HT_ERROR_H

#include <hypetrace.h>
#include <stdint.h>

static inline union HTError HTError_new_no_error(void) {
    union HTError err;
    err.code = HTNoError;
    return err;
}

static inline union HTError HTError_new_end_of_file(void) {
    union HTError err;
    err.code = HTEndOfFile;
    return err;
}

static inline union HTError HTError_new_io(uint64_t pos_row, uint64_t pos_col, int libc_errno) {
    union HTError err;
    err.io.code = HTErrIO;
    err.io.pos_row = pos_row;
    err.io.pos_col = pos_col;
    err.io.libc_errno = libc_errno;
    return err;
}

static inline union HTError HTError_new_quote(uint64_t pos_row, uint64_t pos_col) {
    union HTError err;
    err.quote.code = HTErrQuoteNotClosed;
    err.quote.pos_row = pos_row;
    err.quote.pos_col = pos_col;
    return err;
}

union HTError HTError_new_column(const char *column_name, size_t column_name_len, size_t index_a, size_t index_b);

#endif
