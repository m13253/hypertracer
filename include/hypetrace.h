#ifndef HYPETRACE_H
#define HYPETRACE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HTCsvReader HTCsvReader;

typedef struct HTCsvWriter HTCsvWriter;

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

union HTError HTCsvReader_new(struct HTCsvReader **self, FILE *file);
void HTCsvReader_free(struct HTCsvReader *self);
union HTError HTCsvReader_read_row(struct HTCsvReader *self);
size_t HTCsvReader_num_columns(const struct HTCsvReader *self);
struct HTStrView HTCsvReader_column_name_by_index(const struct HTCsvReader *self, size_t column);
_Bool HTCsvReader_column_index_by_name(const struct HTCsvReader *self, const char *column, size_t column_len, size_t *out);
struct HTStrView HTCsvReader_value_by_column_index(const struct HTCsvReader *self, size_t column);
_Bool HTCsvReader_value_by_column_name(const struct HTCsvReader *self, const char *column, size_t column_len, struct HTStrView *out);

void HTError_free(union HTError *err);
int HTError_print(const union HTError *err, FILE *file);

#ifdef __cplusplus
}
#endif

#endif
