#ifndef HT_CSVREAD_ERROR_H
#define HT_CSVREAD_ERROR_H

#include <hypetrace.h>
#include <stddef.h>
#include <stdint.h>

static inline union HTCsvReadError HTCsvReadError_new_no_error(void) {
    union HTCsvReadError err;
    err.code = HTNoError;
    return err;
}

static inline union HTCsvReadError HTCsvReadError_new_io(uint64_t pos_row, uint64_t pos_col, int libc_errno) {
    union HTCsvReadError err;
    err.code = HTErrIO;
    err.io.pos_row = pos_row;
    err.io.pos_col = pos_col;
    err.io.libc_errno = libc_errno;
    return err;
}

static inline union HTCsvReadError HTCsvReadError_new_quote(uint64_t pos_row, uint64_t pos_col) {
    union HTCsvReadError err;
    err.code = HTErrQuoteNotClosed;
    err.quote.pos_row = pos_row;
    err.quote.pos_col = pos_col;
    return err;
}

union HTCsvReadError HTCsvReadError_new_column(const char *column_name, size_t column_name_len, size_t index_a, size_t index_b);

#endif
