#ifndef HYPETRACE_H
#define HYPETRACE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#define HT_restrict __restrict
#else
#define HT_restrict restrict
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

struct HTCsvWriteError HTCsvWriter_new(struct HTCsvWriter **out, FILE *file, struct HTStrView *header, size_t num_columns);
void HTCsvWriter_free(struct HTCsvWriter *self);
void HTCsvWriter_set_string_by_column_index(struct HTCsvWriter *self, size_t column, char *value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param);
void HTCsvWriter_set_strview_by_column_index(struct HTCsvWriter *self, size_t column, const char *value, size_t value_len);
void HTCsvWriter_set_string_by_column_name(struct HTCsvWriter *self, const char *HT_restrict column, size_t column_len, char *HT_restrict value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param);
void HTCsvWriter_set_strview_by_column_name(struct HTCsvWriter *self, const char *HT_restrict column, size_t column_len, const char *HT_restrict value, size_t value_len);
struct HTCsvWriteError HTCsvWriter_write_row(struct HTCsvWriter *self);

static inline void HTCsvWriteError_free(struct HTCsvWriteError *err) {
    (void) err;
}
void HTCsvWriteError_panic(const struct HTCsvWriteError *err);

#ifdef __cplusplus
}
#endif

#endif
