#include <hypetrace.h>
#include <stdlib.h>
#include <string.h>

static void check_error(HTError *err) {
    if (err->code != HTNoError) {
        fputs("Error: ", stderr);
        HTError_print(err, stderr);
        fputc('\n', stderr);
        HTError_free(err);
        exit(1);
    }
    HTError_free(err);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage %s FILE.csv <COL1> <COL2> ...\n", argv[0]);
        exit(1);
    }
    FILE *file = fopen(argv[1], "rb");
    HTCSVReader *reader;
    HTError err = HTCSVReader_new(&reader, file);
    check_error(&err);
    for (;;) {
        err = HTCSVReader_read_row(reader);
        if (err.code == HTEndOfFile) {
            HTError_free(&err);
            break;
        }
        check_error(&err);
        for (int i = 2; i < argc; i++) {
            HTStrView str;
            if (!HTCSVReader_get_column_by_name(reader, argv[i], strlen(argv[i]), &str)) {
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
    HTCSVReader_free(reader);
    return 0;
}
