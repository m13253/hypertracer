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
#define HT_BOOL     bool
#define HT_RESTRICT __restrict
#else
#define HT_BOOL     _Bool
#define HT_RESTRICT restrict
#endif

typedef struct htCsvReader htCsvReader;
typedef struct htCsvWriter htCsvWriter;
typedef struct htTracer htTracer;

typedef struct htStrView {
    const char *buf;
    size_t len;
} htStrView;

typedef struct htString {
    char *buf;
    size_t len;
    void (*free_func)(void *param);
    void *free_param;
} htString;

typedef struct htLogFile {
    FILE *file;
    struct htStrView filename;
} htLogFile;

typedef enum HTErrorCode {
    HTNoError,
    HTErrIO,
    HTErrQuoteNotClosed,
    HTErrColumnDuplicated,
} HTErrorCode;

typedef union htCsvReadError {
    enum HTErrorCode code;
    union htCsvReadErrorIO {
        enum HTErrorCode code;
        uint64_t pos_row;
        uint64_t pos_col;
        int libc_errno;
    } io;
    union htCsvReadErrorQuote {
        enum HTErrorCode code;
        uint64_t pos_row;
        uint64_t pos_col;
    } quote;
    union htCsvReadErrorColumn {
        enum HTErrorCode code;
        struct htStrView name;
        size_t index_a;
        size_t index_b;
    } column;
} htCsvReadError;

typedef union htCsvWriteError {
    enum HTErrorCode code;
    struct htCsvWriteErrorIO {
        enum HTErrorCode code;
        int libc_errno;
    } io;
} htCsvWriteError;

union htCsvReadError HYPERTRACER_PUBLIC htCsvReader_new(struct htCsvReader **out, FILE *file);
void HYPERTRACER_PUBLIC htCsvReader_free(struct htCsvReader *self);
union htCsvReadError HYPERTRACER_PUBLIC htCsvReader_read_row(struct htCsvReader *self, HT_BOOL *eof);
size_t HYPERTRACER_PUBLIC htCsvReader_num_columns(const struct htCsvReader *self);
struct htStrView HYPERTRACER_PUBLIC htCsvReader_column_name_by_index(const struct htCsvReader *self, size_t column);
HT_BOOL HYPERTRACER_PUBLIC htCsvReader_column_index_by_name(const struct htCsvReader *self, size_t *out, const char *column, size_t column_len);
struct htStrView HYPERTRACER_PUBLIC htCsvReader_value_by_column_index(const struct htCsvReader *self, size_t column);
HT_BOOL HYPERTRACER_PUBLIC htCsvReader_value_by_column_name(const struct htCsvReader *self, struct htStrView *out, const char *column, size_t column_len);

void HYPERTRACER_PUBLIC htCsvReadError_free(union htCsvReadError *err);
void HYPERTRACER_PUBLIC htCsvReadError_panic(const union htCsvReadError *err);

union htCsvWriteError HYPERTRACER_PUBLIC htCsvWriter_new(struct htCsvWriter **out, FILE *file, const struct htStrView header[], size_t num_columns);
void HYPERTRACER_PUBLIC htCsvWriter_free(struct htCsvWriter *self);
void HYPERTRACER_PUBLIC htCsvWriter_set_string_by_column_index(struct htCsvWriter *self, size_t column, char *value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param);
void HYPERTRACER_PUBLIC htCsvWriter_set_strview_by_column_index(struct htCsvWriter *self, size_t column, const char *value, size_t value_len);
void HYPERTRACER_PUBLIC htCsvWriter_set_string_by_column_name(struct htCsvWriter *self, const char *HT_RESTRICT column, size_t column_len, char *value, size_t value_len, void (*value_free_func)(void *param), void *value_free_param);
void HYPERTRACER_PUBLIC htCsvWriter_set_strview_by_column_name(struct htCsvWriter *self, const char *HT_RESTRICT column, size_t column_len, const char *HT_RESTRICT value, size_t value_len);
union htCsvWriteError HYPERTRACER_PUBLIC htCsvWriter_write_row(struct htCsvWriter *self);

static inline void htCsvWriteError_free(union htCsvWriteError *err) {
    (void) err;
}
void HYPERTRACER_PUBLIC htCsvWriteError_panic(const union htCsvWriteError *err);

#define HT_LOG_FILE_DEFAULT_NUM_RETRIES 10
HT_BOOL HYPERTRACER_PUBLIC htLogFile_new(struct htLogFile *out, const char *prefix, size_t prefix_len, unsigned num_retries);
void HYPERTRACER_PUBLIC htLogFile_free(struct htLogFile *self);

#define HT_TRACER_DEFAULT_BUFFER_NUM_ROWS 4096
union htCsvWriteError HYPERTRACER_PUBLIC htTracer_new(struct htTracer **out, FILE *file, const struct htStrView header[], size_t num_str_columns, size_t buffer_num_rows);
union htCsvWriteError HYPERTRACER_PUBLIC htTracer_free(struct htTracer *self);
void HYPERTRACER_PUBLIC htTracer_write_row(struct htTracer *self, struct htString columns[]);

#ifdef __cplusplus
}
#endif

#endif
