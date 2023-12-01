#define __STDC_WANT_LIB_EXT1__ 1
#include "csvread_error.h"
#include "strview.h"
#include <errno.h>
#include <hypetrace.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HTCsvReadError HTCsvReadError_new_column(const char *column_name, size_t column_name_len, size_t index_a, size_t index_b) {
    struct HTCsvReadError err;
    err.code = HTErrColumnDuplicated;
    if (column_name_len == 0) {
        err.column.name = HTStrView_new();
    } else {
        char *buf = malloc(column_name_len);
        if (!buf) {
            abort();
        }
        memcpy(buf, column_name, column_name_len);
        err.column.name = HTStrView_from_HTString(buf, column_name_len);
    }
    err.column.index_a = index_a;
    err.column.index_b = index_b;
    return err;
}

void HTCsvReadError_free(struct HTCsvReadError *err) {
    if (err->code == HTErrColumnDuplicated) {
        free((char *) err->column.name.buf);
    }
}

void HTCsvReadError_panic(const struct HTCsvReadError *err) {
    switch (err->code) {
    case HTNoError:
        return;
    case HTEndOfFile:
        fputs("CsvReadError: end of file\n", stderr);
        break;
    case HTErrIO:
        errno = err->io.libc_errno;
        perror("CsvReadError");
        break;
    case HTErrQuoteNotClosed:
        fprintf(stderr, "CsvReadError: quote not closed at Line %" PRIu64 ", Col %" PRIu64 "\n", err->io.pos_row + 1, err->io.pos_col + 1);
    case HTErrColumnDuplicated: {
        fputs("column \"", stderr);
        fwrite(err->column.name.buf, 1, err->column.name.len, stderr);
        fprintf(stderr, "\" duplicated at both #%zu and #%zu\n", err->column.index_a, err->column.index_b);
        break;
    }
    default:
        fprintf(stderr, "unknown error %d\n", err->code);
        break;
    }
    abort();
}
