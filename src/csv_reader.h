#ifndef HT_CSV_READER_H
#define HT_CSV_READER_H

#include "array.h"
#include "hashmap.h"
#include <stdint.h>
#include <stdio.h>

struct HTCSVReader {
    FILE *file;
    uint64_t pos_row;
    uint64_t pos_col;
    struct HTArray head_buffer;
    struct HTArray line_buffer;
    struct HTHashmap column_index;
};

#endif
