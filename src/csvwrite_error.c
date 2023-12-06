#include "csvwrite_error.h"
#include <errno.h>
#include <hypetrace.h>
#include <stdio.h>
#include <stdlib.h>

void HTCsvWriteError_panic(const struct HTCsvWriteError *err) {
    switch (err->code) {
    case HTNoError:
        return;
    case HTErrIO:
        errno = err->io.libc_errno;
        perror("CsvWriteError");
        break;
    default:
        fprintf(stderr, "unknown error %d\n", err->code);
        break;
    }
    abort();
}
