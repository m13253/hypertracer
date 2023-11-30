#ifndef HT_PARAM_READER_H
#define HT_PARAM_READER_H

#include "array.h"
#include "hashmap.h"
#include <stdint.h>
#include <stdio.h>

struct HTParamReader {
    FILE *file;
    uint64_t pos_row;
    uint64_t pos_col;
    struct HTArray head_buffer;
    struct HTArray line_buffer;
    struct HTHashmap column_index;
};

#endif
