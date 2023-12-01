#include "csvwriter.h"
#include <stdbool.h>
#include <hypetrace.h>

struct HTCsvWriterValue {
    char *buf;
    size_t len;
    void (*free_func)(char *buf, void *extra);
    void *free_extra;
    bool valid;
};
