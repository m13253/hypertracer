#include <hypetrace.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage %s FILE.csv <COL1> <COL2> ...\n", argv[0]);
        exit(1);
    }
    FILE *file = fopen(argv[1], "rb");
    HTCsvReader *reader;
    HTCsvReadError err = HTCsvReader_new(&reader, file);
    HTCsvReadError_panic(&err);
    HTCsvReadError_free(&err);
    for (;;) {
        err = HTCsvReader_read_row(reader);
        if (err.code == HTEndOfFile) {
            HTCsvReadError_free(&err);
            break;
        }
        HTCsvReadError_panic(&err);
        HTCsvReadError_free(&err);
        for (int i = 2; i < argc; i++) {
            HTStrView str;
            if (!HTCsvReader_value_by_column_name(reader, &str, argv[i], strlen(argv[i]))) {
                str.buf = "(N/A)";
                str.len = 5;
            }
            if (i != 2) {
                fputs(", ", stdout);
            }
            printf("%s: \"", argv[i]);
            fwrite(str.buf, 1, str.len, stdout);
            fputs("\"", stdout);
        }
        fputc('\n', stdout);
    }
    HTCsvReader_free(reader);
    return 0;
}
