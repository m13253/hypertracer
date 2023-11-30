#include "error.h"
#include <stdlib.h>
#include <string.h>

void HTError_drop(union HTError *err) {
    if (err->code == HTErrDuplicatedColumn) {
        free((char *) err->dup.column_name.buf);
    }
}

union HTError HTError_new_duplicated_column(const char *column_name, size_t column_name_len, size_t index_a, size_t index_b) {
    union HTError err;
    err.dup.code = HTErrDuplicatedColumn;
    if (column_name_len == 0) {
        err.dup.column_name.buf = NULL;
    } else {
        char *buf = malloc(column_name_len);
        if (!buf) {
            abort();
        }
        memcpy(buf, column_name, column_name_len);
        err.dup.column_name.buf = buf;
    }
    err.dup.column_name.len = column_name_len;
    err.dup.index_a = index_a;
    err.dup.index_b = index_b;
    return err;
}
