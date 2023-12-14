#ifndef HYPERTRACER_CSVREADER_H
#define HYPERTRACER_CSVREADER_H

#include "array.h"
#include "hashmap.h"
#include <stdint.h>
#include <stdio.h>

struct htCsvReader {
    FILE *file;
    uint64_t pos_row;
    uint64_t pos_col;
    struct htArray head_buffer;
    struct htArray line_buffer;
    struct htHashmap column_index;
};

#endif
