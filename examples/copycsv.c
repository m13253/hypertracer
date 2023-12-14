#include <hypertracer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage %s INPUT.csv OUTPUT.csv\n", argv[0]);
        exit(1);
    }
    FILE *fi = fopen(argv[1], "rb");
    htCsvReader *reader;
    {
        htCsvReadError err = htCsvReader_new(&reader, fi);
        htCsvReadError_panic(&err);
        htCsvReadError_free(&err);
    }

    size_t num_columns = htCsvReader_num_columns(reader);
    size_t actual_num_columns = 0;
    size_t num_empty_columns = 0;
    htStrView *column_names = calloc(num_columns, sizeof column_names[0]);
    for (size_t i = 0; i < num_columns; i++) {
        column_names[actual_num_columns] = htCsvReader_column_name_by_index(reader, i);
        // We want at most one empty column
        if (column_names[actual_num_columns].len == 0) {
            if (num_empty_columns == 0) {
                actual_num_columns++;
            }
            num_empty_columns++;
        } else {
            actual_num_columns++;
        }
    }
    num_columns = actual_num_columns;

    FILE *fo = fopen(argv[2], "wb");
    htCsvWriter *writer;
    {
        htCsvWriteError err = htCsvWriter_new(&writer, fo, column_names, num_columns);
        htCsvWriteError_panic(&err);
        htCsvWriteError_free(&err);
    }

    for (;;) {
        {
            bool eof;
            htCsvReadError err = htCsvReader_read_row(reader, &eof);
            htCsvReadError_panic(&err);
            htCsvReadError_free(&err);
            if (eof) {
                break;
            }
        }
        for (size_t i = 0; i < num_columns; i++) {
            htStrView str;
            if (!htCsvReader_value_by_column_name(reader, &str, column_names[i].buf, column_names[i].len)) {
                abort();
            }
            char *dup = malloc(str.len);
            memcpy(dup, str.buf, str.len);
            htCsvWriter_set_string_by_column_name(writer, column_names[i].buf, column_names[i].len, dup, str.len, free, dup);
        }
        {
            htCsvWriteError err = htCsvWriter_write_row(writer);
            htCsvWriteError_panic(&err);
            htCsvWriteError_free(&err);
        }
    }
    free(column_names);
    htCsvReader_free(reader);
    fclose(fi);
    htCsvWriter_free(writer);
    fclose(fo);
    return 0;
}
