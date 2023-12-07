#ifndef HT_CSVWRITE_ERROR_H
#define HT_CSVWRITE_ERROR_H

#include <hypetrace.h>

static inline union HTCsvWriteError HTCsvWriteError_new_no_error(void) {
    union HTCsvWriteError err;
    err.code = HTNoError;
    return err;
}

static inline union HTCsvWriteError HTCsvWriteError_new_io(int libc_errno) {
    union HTCsvWriteError err;
    err.code = HTErrIO;
    err.io.libc_errno = libc_errno;
    return err;
}

#endif
