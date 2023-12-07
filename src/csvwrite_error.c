#include "csvwrite_error.h"
#include <errno.h>
#include <hypertracer.h>
#include <stdio.h>
#include <stdlib.h>

void HTCsvWriteError_panic(const union HTCsvWriteError *err) {
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
