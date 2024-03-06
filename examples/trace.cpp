#include <array>
#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <hypertracer>
#include <iostream>
#include <string_view>
#include <thread>

using namespace std::string_view_literals;

struct ThreadParams {
    std::thread thread;
    ht::Tracer *tracer;
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point end;
    std::chrono::high_resolution_clock::duration interval;
    std::uint64_t trace_per_batch;
};

static void thread_start(ThreadParams &params) noexcept;

int main() {
    ht::Tracer tracer(true, "trace"sv);
    if (tracer.is_enabled()) {
        std::clog << "Writing to: "sv << tracer.get_filename() << ", will take 10 seconds"sv << std::endl;
    } else {
        std::clog << "Will take 10 seconds"sv << std::endl;
    }

    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(10);

    constexpr std::size_t num_threads = 8;

    std::array<ThreadParams, num_threads> params;
    for (std::size_t i = 0; i < num_threads; i++) {
        params.at(i).tracer = &tracer;
        params.at(i).start = start;
        params.at(i).end = end;
        params.at(i).interval = std::chrono::milliseconds(static_cast<std::int64_t>(i) + 1);
        params.at(i).trace_per_batch = 5;
    }
    for (std::size_t i = 0; i < num_threads; i++) {
        params.at(i).thread = std::thread(thread_start, std::ref(params.at(i)));
    }
    for (std::size_t i = 0; i < num_threads; i++) {
        params.at(i).thread.join();
    }
}

static void thread_start(ThreadParams &params) noexcept {
    ht::Event ev(*params.tracer, "thread_start"sv, "func"sv, true);

    auto interval_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(params.interval).count();
    if (interval_nsec < 0) {
        std::terminate();
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
            ht::Event(*params.tracer, "trace"sv, "func,trace"sv, false)
                .set_args([batch_id, trace_id](ht::PayloadMap &payload) {
                    payload.push("batch_id"sv, batch_id);
                    payload.push_array("trace_id"sv, [batch_id, trace_id](ht::PayloadArray &payload) {
                        payload.push(batch_id);
                        payload.push(trace_id);
                    });
                });
        }
    }
}
