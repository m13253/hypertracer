#ifndef HYPERTRACER_H
#define HYPERTRACER_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_HYPERTRACER
#define HYPERTRACER_PUBLIC __declspec(dllexport)
#else
#define HYPERTRACER_PUBLIC __declspec(dllimport)
#endif
#else
#ifdef BUILDING_HYPERTRACER
#define HYPERTRACER_PUBLIC __attribute__((visibility("default")))
#else
#define HYPERTRACER_PUBLIC
#endif
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

typedef struct HTString {
    char *buf;
    size_t len;
    void (*free_func)(void *param);
    void *free_param;
} HTString;

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

union HTCsvReadError HYPERTRACER_PUBLIC HTCsvReader_new(struct HTCsvReader **out, FILE *file);
void HYPERTRACER_PUBLIC HTCsvReader_free(struct HTCsvReader *self);
union HTCsvReadError HYPERTRACER_PUBLIC HTCsvReader_read_row(struct HTCsvReader *self, HT_bool *eof);
size_t HYPERTRACER_PUBLIC HTCsvReader_num_columns(const struct HTCsvReader *self);
struct HTStrView HYPERTRACER_PUBLIC HTCsvReader_column_name_by_index(const struct HTCsvReader *self, size_t column);
HT_bool HYPERTRACER_PUBLIC HTCsvReader_column_index_by_name(const struct HTCsvReader *self, size_t *out, const char *column, size_t column_len);
struct HTStrView HYPERTRACER_PUBLIC HTCsvReader_value_by_column_index(const struct HTCsvReader *self, size_t column);
HT_bool HYPERTRACER_PUBLIC HTCsvReader_value_by_column_name(const struct HTCsvReader *self, struct HTStrView *out, const char *column, size_t column_len);

void HYPERTRACER_PUBLIC HTCsvReadError_free(union HTCsvReadError *err);
void HYPERTRACER_PUBLIC HTCsvReadError_panic(const union HTCsvReadError *err);

union HTCsvWriteError HYPERTRACER_PUBLIC HTCsvWriter_new(struct HTCsvWriter **out, FILE *file, const struct HTStrView header[], size_t num_columns);
void HYPERTRACER_PUBLIC HTCsvWriter_free(struct HTCsvWriter *self);
void HYPERTRACER_PUBLIC HTCsvWriter_set_string_by_column_index(struct HTCsvWriter *self, size_t column, char *value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param);
void HYPERTRACER_PUBLIC HTCsvWriter_set_strview_by_column_index(struct HTCsvWriter *self, size_t column, const char *value, size_t value_len);
void HYPERTRACER_PUBLIC HTCsvWriter_set_string_by_column_name(struct HTCsvWriter *self, const char *HT_restrict column, size_t column_len, char *value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param);
void HYPERTRACER_PUBLIC HTCsvWriter_set_strview_by_column_name(struct HTCsvWriter *self, const char *HT_restrict column, size_t column_len, const char *HT_restrict value, size_t value_len);
union HTCsvWriteError HYPERTRACER_PUBLIC HTCsvWriter_write_row(struct HTCsvWriter *self);

static inline void HTCsvWriteError_free(union HTCsvWriteError *err) {
    (void) err;
}
void HYPERTRACER_PUBLIC HTCsvWriteError_panic(const union HTCsvWriteError *err);

#define HT_LOG_FILE_DEFAULT_NUM_RETRIES 10
HT_bool HYPERTRACER_PUBLIC HTLogFile_new(struct HTLogFile *out, const char *prefix, size_t prefix_len, unsigned num_retries);
void HYPERTRACER_PUBLIC HTLogFile_free(struct HTLogFile *self);

#define HT_TRACER_DEFAULT_BUFFER_NUM_ROWS 4096
union HTCsvWriteError HYPERTRACER_PUBLIC HTTracer_new(struct HTTracer **out, FILE *file, const struct HTStrView header[], size_t num_str_columns, size_t buffer_num_rows);
union HTCsvWriteError HYPERTRACER_PUBLIC HTTracer_free(struct HTTracer *self);
void HYPERTRACER_PUBLIC HTTracer_write_row(struct HTTracer *self, struct HTString columns[]);

#ifdef __cplusplus
}
#endif

#endif
