#include <array>
#include <atomic>
#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <hypertracer>
#include <iostream>
#include <ratio>
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
    std::atomic_unsigned_lock_free *counter;
};

static void thread_start(ThreadParams &params) noexcept;
static void controlled_sleep(std::chrono::high_resolution_clock::time_point &last_wake, const std::chrono::high_resolution_clock::duration &duration) noexcept;

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
    std::atomic_unsigned_lock_free counter = 0;

    std::array<ThreadParams, num_threads> params;
    for (std::size_t i = 0; i < num_threads; i++) {
        params.at(i).tracer = &tracer;
        params.at(i).start = start;
        params.at(i).end = end;
        params.at(i).interval = std::chrono::milliseconds(static_cast<std::int64_t>(i) + 1);
        params.at(i).trace_per_batch = 5;
        params.at(i).counter = &counter;
    }
    for (std::size_t i = 0; i < num_threads; i++) {
        params.at(i).thread = std::thread(thread_start, std::ref(params.at(i)));
    }
    for (std::size_t i = 0; i < num_threads; i++) {
        params.at(i).thread.join();
    }
}

static void thread_start(ThreadParams &params) noexcept {
    ht::Event ev(*params.tracer, "thread_start"sv, "func"sv, ht::EventType::Duration);

    auto last_wake = params.start;
    for (std::uint64_t batch_id = 0;; batch_id++) {
        controlled_sleep(last_wake, params.interval);
        if (last_wake >= params.end) {
            return;
        }

        ht::Event ev(*params.tracer, "batch"sv, "func,trace"sv, ht::EventType::Duration);
        ev.set_args([batch_id](ht::PayloadMap &payload) {
            payload.push("batch_id"sv, batch_id);
        });

        for (std::uint64_t trace_id = 0; trace_id < params.trace_per_batch; trace_id++) {
            if (trace_id != 0) {
                controlled_sleep(last_wake, params.interval);
                if (last_wake >= params.end) {
                    return;
                }
            }

            ht::Event(*params.tracer, "trace"sv, "func,trace"sv, ht::EventType::Instant)
                .set_args([batch_id, trace_id](ht::PayloadMap &payload) {
                    payload.push("batch_id"sv, batch_id);
                    payload.push_array("trace_id"sv, [batch_id, trace_id](ht::PayloadArray &payload) {
                        payload.push(batch_id);
                        payload.push(trace_id);
                    });
                });

            std::uint64_t counter = params.counter->fetch_add(1, std::memory_order::relaxed);
            ht::Event(*params.tracer, "trace"sv, "func,trace,counter"sv, ht::EventType::Counter)
                .set_args([counter](ht::PayloadMap &payload) {
                    payload.push("counter"sv, counter);
                });
        }
    }
}

static void controlled_sleep(std::chrono::high_resolution_clock::time_point &last_wake, const std::chrono::high_resolution_clock::duration &duration) noexcept {
    auto next_wake = last_wake + duration;
    auto now = std::chrono::high_resolution_clock::now();
    auto diff = next_wake - now;
    if (diff > diff.zero()) {
        std::this_thread::sleep_for(diff);
        last_wake = next_wake;
    } else {
        last_wake = now;
    }
}
