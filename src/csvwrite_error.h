#ifndef HT_CSVWRITE_ERROR_H
#define HT_CSVWRITE_ERROR_H

#include <hypetrace.h>

static inline struct HTCsvWriteError HTCsvWriteError_new_no_error(void) {
    struct HTCsvWriteError err;
    err.code = HTNoError;
    return err;
}

static inline struct HTCsvWriteError HTCsvWriteError_new_io(int libc_errno) {
    struct HTCsvWriteError err;
    err.code = HTErrIO;
    err.io.libc_errno = libc_errno;
    return err;
}

#endif
