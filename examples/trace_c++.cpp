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
    std::uint64_t trace_per_batch;
};

static void thread_start(ThreadParams &params) {
    auto interval_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(params.interval).count();
    if (interval_nsec < 0) {
        std::abort();
    }

    for (std::uint64_t batch_id = 0;; batch_id++) {
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

        for (std::uint64_t trace_id = 0; trace_id < params.trace_per_batch; trace_id++) {
            std::ostringstream ss;
            ss << std::this_thread::get_id();
            params.tracer->write_row({
                new std::string(ss.str()),
                new std::string(std::to_string(batch_id)),
                new std::string(std::to_string(trace_id)),
            });
        }
    }
}

int main() {
    ht::LogFile log_file("trace"sv);
    std::cout << "Writing to: "sv << log_file.filename() << ", will take 10 seconds"sv << std::endl;

    ht::Tracer tracer(log_file, {"thread_id"sv, "batch_id"sv, "trace_id"sv}, 8);

    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(10);

    std::array<ThreadParams, 16> params;
    for (std::size_t i = 0; i < 16; i++) {
        params.at(i).tracer = &tracer;
        params.at(i).start = start;
        params.at(i).end = end;
        params.at(i).interval = std::chrono::milliseconds(static_cast<std::int64_t>(i) + 1);
        params.at(i).trace_per_batch = 5;
    }
    for (std::size_t i = 0; i < 16; i++) {
        params.at(i).thread = std::thread(thread_start, std::ref(params.at(i)));
    }
    for (std::size_t i = 0; i < 16; i++) {
        params.at(i).thread.join();
    }
}
