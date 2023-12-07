#include <hypetrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct HTLogFile HTLogFile_new(const char *prefix, size_t prefix_len, unsigned num_retries) {
    size_t suffix_max_len = sizeof "_0000-00-00T00-00-00Z.csv";
    if (prefix_len >= SIZE_MAX - suffix_max_len) {
        fputs("panic: HTLogFile_new: filename too long\n", stderr);
        abort();
    }
    for (size_t i = 0; i < prefix_len; i++) {
        if (prefix[i] == '\x00') {
            fputs("panic: HTLogFile_new: filename contains NUL bytes\n", stderr);
            abort();
        }
    }

    struct HTLogFile self;
    self.filename.buf = malloc(self.filename.len + 1);
    if (!self.filename.buf) {
        abort();
    }
    memcpy(self.filename.buf, prefix, prefix_len);

    time_t timer = time(NULL);
    if (timer == (time_t) -1) {
        fputs("panic: HTLogFile_new: failed to get current date and time\n", stderr);
        abort();
    }
    while (num_retries-- != 0) {
        struct tm tm;
        struct tm *ptm = gmtime_r(&timer, &tm);
        if (ptm == NULL) {
            fputs("panic: HTLogFile_new: failed to get current date and time\n", stderr);
            abort();
        }
        size_t suffix_len = strftime(&self.filename.buf[prefix_len], suffix_max_len + 1, "_%Y-%m-%dT%H-%M-%SZ.csv", ptm);
        self.file = fopen(self.filename.buf, "wxb");
        if (self.file) {
            self.filename.len = prefix_len + suffix_len;
            return self;
        }
        timer++;
    }
    free((char *) self.filename.buf);
    self.filename.buf = NULL;
    self.filename.len = 0;
    return self;
}

void HTLogFile_free(struct HTLogFile *self) {
    fclose(self->file);
    free((char *) self->filename.buf);
}
