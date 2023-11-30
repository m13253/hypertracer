#ifndef HT_CSVWRITER_H
#define HT_CSVWRITER_H

#include "hashmap.h"
#include "string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct HTCsvWriter {
    FILE *file;
    size_t num_columns;
    struct HTString *head_buffer;
    struct HTCsvWriterValue *line_buffer;
    struct HTHashmap column_index;
};

#endif
