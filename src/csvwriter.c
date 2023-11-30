#include "csvwriter.h"
#include "hypetrace.h"

enum HTCsvWriterValueState {
    StateEmpty,
    StateOwned,
    StateView,
};

struct HTCsvWriterValue {
    enum HTCsvWriterValueState state;
    union {
        struct HTString owned;
        struct HTStrView view;
    };
};
