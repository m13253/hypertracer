#include <hypetrace.h>
#include <stdlib.h>
#include <string.h>

static void check_error(HTError *err) {
    if (err->code != HTNoError) {
        fputs("Error: ", stderr);
        HTError_print(err, stderr);
        fputc('\n', stderr);
        HTError_drop(err);
        exit(1);
    }
    HTError_drop(err);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage %s FILE.csv <COL1> <COL2> ...\n", argv[0]);
        exit(1);
    }
    FILE *file = fopen(argv[1], "rb");
    HTParamReader *reader;
    HTError err = HTParamReader_new(&reader, file);
    check_error(&err);
    for (;;) {
        err = HTParamReader_read_row(reader);
        if (err.code == HTEndOfFile) {
            HTError_drop(&err);
            break;
        }
        check_error(&err);
        for (int i = 2; i < argc; i++) {
            HTStrView str;
            if (!HTParamReader_get(reader, argv[i], strlen(argv[i]), &str)) {
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
    HTParamReader_drop(reader);
    return 0;
}
