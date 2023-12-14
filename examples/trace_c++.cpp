#include <array>
#include <chrono>
#include <compare>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <hypertracer>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <variant>

using namespace std::literals;

struct ThreadParams {
    std::thread thread;
    ht::Tracer *tracer;
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point end;
    std::chrono::high_resolution_clock::duration interval;
    uint64_t trace_per_batch;
};

static void thread_start(ThreadParams &params) {
    auto interval_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(params.interval).count();
    if (interval_nsec < 0) {
        std::abort();
    }

    for (uint64_t batch_id = 0;; batch_id++) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now >= params.end) {
            break;
        }

        auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(now - params.start).count();
        if (diff <= 0) {
            break;
        }
        auto sleep_nsec = interval_nsec - (diff % interval_nsec);
        std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_nsec));

        for (uint64_t trace_id = 0; trace_id < params.trace_per_batch; trace_id++) {
            std::array<std::variant<std::string_view, std::string *>, 3> trace;
            {
                std::ostringstream ss;
                ss << std::this_thread::get_id();
                trace.at(0).emplace<std::string *>(new std::string(ss.str()));
            }
            trace.at(1).emplace<std::string *>(new std::string(std::to_string(batch_id)));
            trace.at(2).emplace<std::string *>(new std::string(std::to_string(trace_id)));
            params.tracer->write_row(trace);
        }
    }
}

int main() {
    ht::LogFile log_file("trace"sv);
    std::cout << "Writing to: "sv << log_file.filename() << ", will take 10 seconds"sv << std::endl;

    std::array<std::string_view, 3> header{"thread_id"sv, "batch_id"sv, "trace_id"sv};
    ht::Tracer tracer(log_file, header, 8);

    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(10);

    std::array<ThreadParams, 16> params;
    for (size_t i = 0; i < 16; i++) {
        params.at(i).tracer = &tracer;
        params.at(i).start = start;
        params.at(i).end = end;
        params.at(i).interval = std::chrono::milliseconds(static_cast<int64_t>(i) + 1);
        params.at(i).trace_per_batch = 5;
    }
    for (size_t i = 0; i < 16; i++) {
        params.at(i).thread = std::thread(thread_start, std::ref(params.at(i)));
    }
    for (size_t i = 0; i < 16; i++) {
        params.at(i).thread.join();
    }
}
