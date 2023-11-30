#ifndef HYPETRACE_H
#define HYPETRACE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct HTParamReader HTParamReader;

typedef struct HTTraceWriter HTTraceWriter;

typedef struct HTStrView {
    const char *buf;
    size_t len;
} HTStrView;

typedef enum HTErrorCode {
    HTNoError,
    HTEndOfFile,
    HTErrIO,
    HTErrParsingQuoteNotClosed,
    HTErrParsingTooFewColumns,
    HTErrParsingTooManyColumns,
    HTErrDuplicatedColumn,
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
    } parser;
    struct {
        enum HTErrorCode code;
        struct HTStrView column_name;
        size_t index_a;
        size_t index_b;
    } dup;
} HTError;

union HTError HTParamReader_new(struct HTParamReader *self, FILE *file);
void HTParamReader_drop(struct HTParamReader *self);
union HTError HTParamReader_read_row(struct HTParamReader *self);
_Bool HTParamReader_get(struct HTParamReader *self, const char *column, size_t column_len, struct HTStrView *out);

#endif
