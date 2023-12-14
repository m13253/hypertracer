#ifndef HYPERTRACER_CSVWRITER_H
#define HYPERTRACER_CSVWRITER_H

#include "hashmap.h"
#include <stdio.h>

struct htCsvWriter {
    FILE *file;
    size_t num_columns;
    struct htCsvWriterValue *line_buffer;
    struct htHashmap column_index;
};

#endif
