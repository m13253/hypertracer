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
    HTCsvReader *reader;
    {
        HTCsvReadError err = HTCsvReader_new(&reader, fi);
        HTCsvReadError_panic(&err);
        HTCsvReadError_free(&err);
    }

    size_t num_columns = HTCsvReader_num_columns(reader);
    size_t actual_num_columns = 0;
    size_t num_empty_columns = 0;
    HTStrView *column_names = calloc(num_columns, sizeof column_names[0]);
    for (size_t i = 0; i < num_columns; i++) {
        column_names[actual_num_columns] = HTCsvReader_column_name_by_index(reader, i);
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
    HTCsvWriter *writer;
    {
        HTCsvWriteError err = HTCsvWriter_new(&writer, fo, column_names, num_columns);
        HTCsvWriteError_panic(&err);
        HTCsvWriteError_free(&err);
    }

    for (;;) {
        {
            bool eof;
            HTCsvReadError err = HTCsvReader_read_row(reader, &eof);
            HTCsvReadError_panic(&err);
            HTCsvReadError_free(&err);
            if (eof) {
                break;
            }
        }
        for (size_t i = 0; i < num_columns; i++) {
            HTStrView str;
            if (!HTCsvReader_value_by_column_name(reader, &str, column_names[i].buf, column_names[i].len)) {
                abort();
            }
            char *dup = malloc(str.len);
            memcpy(dup, str.buf, str.len);
            HTCsvWriter_set_string_by_column_name(writer, column_names[i].buf, column_names[i].len, dup, str.len, free, dup);
        }
        {
            HTCsvWriteError err = HTCsvWriter_write_row(writer);
            HTCsvWriteError_panic(&err);
            HTCsvWriteError_free(&err);
        }
    }
    free(column_names);
    HTCsvReader_free(reader);
    fclose(fi);
    HTCsvWriter_free(writer);
    fclose(fo);
    return 0;
}
