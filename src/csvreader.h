#ifndef HT_CSVREADER_H
#define HT_CSVREADER_H

#include "array.h"
#include "hashmap.h"
#include <stdint.h>
#include <stdio.h>

struct HTCsvReader {
    FILE *file;
    uint64_t pos_row;
    uint64_t pos_col;
    struct HTArray head_buffer;
    struct HTArray line_buffer;
    struct HTHashmap column_index;
};

#endif
