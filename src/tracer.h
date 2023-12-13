#include "mpsc.h"
#include "csvwriter.h"
#include <hypertracer.h>
#include <threads.h>

#ifndef HT_TRACER_H
#define HT_TRACER_H

struct HTTracer {
    struct HTMpsc chan;
    struct HTCsvWriter *csv_writer;
    union HTCsvWriteError err;
    thrd_t write_thread;
};

#endif
