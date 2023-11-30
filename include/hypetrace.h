#ifndef HYPETRACE_H
#define HYPETRACE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct HTCSVReader HTCSVReader;

typedef struct HTCSVWriter HTCSVWriter;

typedef struct HTStrView {
    const char *buf;
    size_t len;
} HTStrView;

typedef enum HTErrorCode {
    HTNoError,
    HTEndOfFile,
    HTErrIO,
    HTErrQuoteNotClosed,
    HTErrColumnDuplicated,
} HTErrorCode;

typedef union HTError {
    enum HTErrorCode code;
    struct {
        enum HTErrorCode code;
        uint64_t pos_row;
        uint64_t pos_col;
        int libc_errno;
    } io;
    struct {
        enum HTErrorCode code;
        uint64_t pos_row;
        uint64_t pos_col;
    } quote;
    struct {
        enum HTErrorCode code;
        struct HTStrView name;
        size_t index_a;
        size_t index_b;
    } column;
} HTError;

union HTError HTCSVReader_new(struct HTCSVReader **self, FILE *file);
void HTCSVReader_free(struct HTCSVReader *self);
union HTError HTCSVReader_read_row(struct HTCSVReader *self);
_Bool HTCSVReader_get_column_by_name(struct HTCSVReader *self, const char *column, size_t column_len, struct HTStrView *out);

void HTError_free(union HTError *err);
int HTError_print(const union HTError *err, FILE *file);

#endif
