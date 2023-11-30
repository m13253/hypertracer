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

typedef struct HTCsvReadError {
    enum HTErrorCode code;
    union {
        struct {
            uint64_t pos_row;
            uint64_t pos_col;
            int libc_errno;
        } io;
        struct {
            uint64_t pos_row;
            uint64_t pos_col;
        } quote;
        struct {
            struct HTStrView name;
            size_t index_a;
            size_t index_b;
        } column;
    };
} HTCsvReadError;

typedef struct HTCsvWriteError {
    enum HTErrorCode code;
    struct {
        int libc_errno;
    } io;
} HTCsvWriteError;

struct HTCsvReadError HTCsvReader_new(struct HTCsvReader **out, FILE *file);
void HTCsvReader_free(struct HTCsvReader *self);
struct HTCsvReadError HTCsvReader_read_row(struct HTCsvReader *self);
size_t HTCsvReader_num_columns(const struct HTCsvReader *self);
struct HTStrView HTCsvReader_column_name_by_index(const struct HTCsvReader *self, size_t column);
_Bool HTCsvReader_column_index_by_name(const struct HTCsvReader *self, size_t *out, const char *column, size_t column_len);
struct HTStrView HTCsvReader_value_by_column_index(const struct HTCsvReader *self, size_t column);
_Bool HTCsvReader_value_by_column_name(const struct HTCsvReader *self, struct HTStrView *out, const char *column, size_t column_len);

void HTCsvReadError_free(struct HTCsvReadError *err);
void HTCsvReadError_panic(const struct HTCsvReadError *err);

#ifdef __cplusplus
}
#endif

#endif
