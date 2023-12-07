#ifndef HYPETRACE_H
#define HYPETRACE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#define HT_bool     bool
#define HT_restrict __restrict
#else
#define HT_bool     _Bool
#define HT_restrict restrict
#endif

typedef struct HTCsvReader HTCsvReader;

typedef struct HTCsvWriter HTCsvWriter;

typedef struct HTTracer HTTracer;

typedef struct HTStrView {
    const char *buf;
    size_t len;
} HTStrView;

typedef struct HTLogFile {
    FILE *file;
    struct HTStrView filename;
} HTLogFile;

typedef enum HTErrorCode {
    HTNoError,
    HTErrIO,
    HTErrQuoteNotClosed,
    HTErrColumnDuplicated,
} HTErrorCode;

typedef union HTCsvReadError {
    enum HTErrorCode code;
    union HTCsvReadErrorIO {
        enum HTErrorCode code;
        uint64_t pos_row;
        uint64_t pos_col;
        int libc_errno;
    } io;
    union HTCsvReadErrorQuote {
        enum HTErrorCode code;
        uint64_t pos_row;
        uint64_t pos_col;
    } quote;
    union HTCsvReadErrorColumn {
        enum HTErrorCode code;
        struct HTStrView name;
        size_t index_a;
        size_t index_b;
    } column;
} HTCsvReadError;

typedef union HTCsvWriteError {
    enum HTErrorCode code;
    struct HTCsvWriteErrorIO {
        enum HTErrorCode code;
        int libc_errno;
    } io;
} HTCsvWriteError;

union HTCsvReadError HTCsvReader_new(struct HTCsvReader **out, FILE *file);
void HTCsvReader_free(struct HTCsvReader *self);
union HTCsvReadError HTCsvReader_read_row(struct HTCsvReader *self, HT_bool *eof);
size_t HTCsvReader_num_columns(const struct HTCsvReader *self);
struct HTStrView HTCsvReader_column_name_by_index(const struct HTCsvReader *self, size_t column);
HT_bool HTCsvReader_column_index_by_name(const struct HTCsvReader *self, size_t *out, const char *column, size_t column_len);
struct HTStrView HTCsvReader_value_by_column_index(const struct HTCsvReader *self, size_t column);
HT_bool HTCsvReader_value_by_column_name(const struct HTCsvReader *self, struct HTStrView *out, const char *column, size_t column_len);

void HTCsvReadError_free(union HTCsvReadError *err);
void HTCsvReadError_panic(const union HTCsvReadError *err);

union HTCsvWriteError HTCsvWriter_new(struct HTCsvWriter **out, FILE *file, struct HTStrView *header, size_t num_columns);
void HTCsvWriter_free(struct HTCsvWriter *self);
void HTCsvWriter_set_string_by_column_index(struct HTCsvWriter *self, size_t column, char *value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param);
void HTCsvWriter_set_strview_by_column_index(struct HTCsvWriter *self, size_t column, const char *value, size_t value_len);
void HTCsvWriter_set_string_by_column_name(struct HTCsvWriter *self, const char *HT_restrict column, size_t column_len, char *value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param);
void HTCsvWriter_set_strview_by_column_name(struct HTCsvWriter *self, const char *HT_restrict column, size_t column_len, const char *HT_restrict value, size_t value_len);
union HTCsvWriteError HTCsvWriter_write_row(struct HTCsvWriter *self);

static inline void HTCsvWriteError_free(union HTCsvWriteError *err) {
    (void) err;
}
void HTCsvWriteError_panic(const union HTCsvWriteError *err);

HT_bool HTLogFile_new(struct HTLogFile *out, const char *prefix, size_t prefix_len, unsigned num_retries);
void HTLogFile_free(struct HTLogFile *self);

#ifdef __cplusplus
}
#endif

#endif
