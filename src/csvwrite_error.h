#ifndef HYPERTRACER_CSVWRITE_ERROR_H
#define HYPERTRACER_CSVWRITE_ERROR_H

#include <hypertracer.h>

static inline union htCsvWriteError htCsvWriteError_new_no_error(void) {
    union htCsvWriteError err;
    err.code = HTNoError;
    return err;
}

static inline union htCsvWriteError htCsvWriteError_new_io(int libc_errno) {
    union htCsvWriteError err;
    err.code = HTErrIO;
    err.io.libc_errno = libc_errno;
    return err;
}

#endif
