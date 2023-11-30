#define __STDC_WANT_LIB_EXT1__ 1
#include "error.h"
#include "hypetrace.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

union HTError HTError_new_column(const char *column_name, size_t column_name_len, size_t index_a, size_t index_b) {
    union HTError err;
    err.column.code = HTErrColumnDuplicated;
    if (column_name_len == 0) {
        err.column.name.buf = NULL;
    } else {
        char *buf = malloc(column_name_len);
        if (!buf) {
            abort();
        }
        memcpy(buf, column_name, column_name_len);
        err.column.name.buf = buf;
    }
    err.column.name.len = column_name_len;
    err.column.index_a = index_a;
    err.column.index_b = index_b;
    return err;
}

void HTError_free(union HTError *err) {
    if (err->code == HTErrColumnDuplicated) {
        free((char *) err->column.name.buf);
    }
}

int HTError_print(const union HTError *err, FILE *file) {
    switch (err->code) {
    case HTNoError:
        return fprintf(file, "no error");
    case HTEndOfFile:
        return fprintf(file, "end of file");
    case HTErrIO: {
#ifdef __STDC_LIB_EXT1__
        size_t size = strerrorlen_s(err->io.libc_errno);
        char *buf = malloc(size + 1);
        if (!buf) {
            abort();
        }
        int result;
        if (strerror_r(err->io.libc_errno, buf, size + 1) != 0) {
            result = fprintf(file, "I/O error at row %" PRIu64 " column %" PRIu64 ": error %d", err->io.pos_row + 1, err->io.pos_col + 1, err->io.libc_errno);
        } else {
            result = fprintf(file, "I/O error at row %" PRIu64 " column %" PRIu64 ": %s", err->io.pos_row + 1, err->io.pos_col + 1, buf);
        }
        free(buf);
        return result;
#else
        const char *buf = strerror(err->io.libc_errno);
        if (!buf) {
            return fprintf(file, "I/O error at row %" PRIu64 " column %" PRIu64 ": error %d", err->io.pos_row + 1, err->io.pos_col + 1, err->io.libc_errno);
        } else {
            return fprintf(file, "I/O error at row %" PRIu64 " column %" PRIu64 ": %s", err->io.pos_row + 1, err->io.pos_col + 1, buf);
        }
#endif
    }
    case HTErrQuoteNotClosed:
        return fprintf(file, "parsing error at row %" PRIu64 " column %" PRIu64 ": quote not closed", err->io.pos_row + 1, err->io.pos_col + 1);
    case HTErrColumnDuplicated: {
        int count1 = fprintf(file, "column \"");
        if (count1 < 0) {
            return count1;
        }
        size_t count2 = fwrite(err->column.name.buf, 1, err->column.name.len, stderr);
        if (count2 != err->column.name.len) {
            return -1;
        }
        int count3 = fprintf(file, "\" duplicated at both #%zu and #%zu", err->column.index_a, err->column.index_b);
        if (count3 < 0) {
            return count3;
        }
        return count1 + (int) count2 + count3;
    }
    default:
        return fprintf(file, "unknown error %d", err->code);
    }
}
