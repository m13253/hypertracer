#include <hypertracer.h>
#include <inttypes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

struct ThreadParams {
    pthread_t thread;
    HTTracer *tracer;
    struct timespec start;
    struct timespec end;
    int64_t interval_nsec;
    uint64_t trace_per_batch;
};

static void *thread_start(void *ptr) {
    struct ThreadParams *params = ptr;

    if (params->interval_nsec <= 0) {
        abort();
    }

    for (int64_t trace_id = 0;; trace_id++) {
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            perror("clock_gettime");
            abort();
        }
        if (
            now.tv_sec >= params->end.tv_sec ||
            (now.tv_sec == params->end.tv_sec && now.tv_nsec >= params->end.tv_nsec)
        ) {
            break;
        }

        int sleep_result;
        do {
            int64_t diff = (now.tv_sec - params->start.tv_sec) * (int64_t) 1000000000 + (now.tv_nsec - params->start.tv_nsec);
            int64_t sleep_nsec = diff == 0 ? 0 : params->interval_nsec - diff % params->interval_nsec;
            struct timespec sleep_req;
            sleep_req.tv_sec = sleep_nsec / 1000000000;
            sleep_req.tv_nsec = sleep_nsec % 1000000000;
            sleep_result = nanosleep(&sleep_req, NULL);
            if (sleep_result < -1) {
                perror("nanosleep");
                abort();
            }
        } while (sleep_result != 0);

        for (uint64_t i = 0; i < params->trace_per_batch; i++) {
            HTString trace[3];
            uint64_t thread_id;
            if (pthread_threadid_np(pthread_self(), &thread_id) != 0) {
                perror("pthread_threadid_np");
                abort();
            }
            int len = asprintf(&trace[0].buf, "%" PRIu64, thread_id);
            if (len < 0) {
                perror("asprintf");
                abort();
            }
            trace[0].len = len;
            trace[0].free_func = free;
            trace[0].free_param = trace[0].buf;
            len = asprintf(&trace[1].buf, "%" PRIu64, trace_id);
            if (len < 0) {
                perror("asprintf");
                free(trace[0].buf);
                abort();
            }
            trace[1].len = len;
            trace[1].free_func = free;
            trace[1].free_param = trace[1].buf;
            len = asprintf(&trace[2].buf, "%" PRIu64, i);
            if (len < 0) {
                perror("asprintf");
                free(trace[0].buf);
                free(trace[1].buf);
                abort();
            }
            trace[2].len = len;
            trace[2].free_func = free;
            trace[2].free_param = trace[2].buf;
            HTTracer_write_row(params->tracer, trace);
        }
    }
    return NULL;
}

int main(void) {
    struct HTLogFile log_file;
    if (!HTLogFile_new(&log_file, "trace", 5, HT_LOG_FILE_DEFAULT_NUM_RETRIES)) {
        fputs("Unable to create output file\n", stderr);
        abort();
    }
    fputs("Writing to: ", stdout);
    fwrite(log_file.filename.buf, 1, log_file.filename.len, stdout);
    fputs(", will take 10 seconds\n", stdout);

    struct HTStrView header[3];
    header[0].buf = "thread_id";
    header[0].len = 9;
    header[1].buf = "batch_id";
    header[1].len = 8;
    header[2].buf = "trace_id";
    header[2].len = 8;
    struct HTTracer *tracer;
    union HTCsvWriteError err = HTTracer_new(&tracer, log_file.file, header, 3, HT_TRACER_DEFAULT_BUFFER_NUM_ROWS);
    HTCsvWriteError_panic(&err);
    HTCsvWriteError_free(&err);

    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("clock_gettime");
        abort();
    }
    struct timespec end;
    end.tv_sec = start.tv_sec + 10;
    end.tv_nsec = start.tv_nsec;

    struct ThreadParams params[16];
    for (int i = 0; i < 16; i++) {
        params[i].tracer = tracer;
        params[i].start = start;
        params[i].end = end;
        params[i].interval_nsec = ((int64_t) i + 1) * 1000000;
        params[i].trace_per_batch = 5;
    }
    for (int i = 0; i < 16; i++) {
        if (pthread_create(&params[i].thread, NULL, thread_start, &params[i]) != 0) {
            perror("pthread_create");
            abort();
        }
    }
    for (int i = 0; i < 16; i++) {
        void *retval;
        if (pthread_join(params[i].thread, &retval) != 0) {
            perror("pthread_join");
            abort();
        }
        if (retval != NULL) {
            abort();
        }
    }

    err = HTTracer_free(tracer);
    HTCsvWriteError_panic(&err);
    HTCsvWriteError_free(&err);
    HTLogFile_free(&log_file);

    return 0;
}