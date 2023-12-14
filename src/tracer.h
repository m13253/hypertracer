#ifndef HYPERTRACER_TRACER_H
#define HYPERTRACER_TRACER_H

#include "mpsc.h"
#include <hypertracer.h>
#include <threads.h>

struct htTracer {
    struct htMpsc chan;
    struct htCsvWriter *csv_writer;
    union htCsvWriteError err;
    thrd_t write_thread;
};

#endif
