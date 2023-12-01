#ifndef HT_CSVWRITER_H
#define HT_CSVWRITER_H

#include "hashmap.h"
#include <stdio.h>

struct HTCsvWriter {
    FILE *file;
    size_t num_columns;
    struct HTStrView *head_buffer;
    struct HTCsvWriterValue *line_buffer;
    struct HTHashmap column_index;
};

#endif
