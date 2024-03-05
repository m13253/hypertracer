#include "hypertracer"
#include "file"
#include "msg"
#include "panic"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cerrno>
#include <exception>
#include <pthread.h>
#include <string_view>
#include <system_error>
#include <time.h>

namespace ht {

void Tracer::Internal::thread_main(std::string_view prefix, std::string_view suffix, std::size_t buffer_size, std::promise<FileInfo> info, std::promise<void> error) noexcept {
    using namespace std::string_view_literals;

#ifdef __linux__
    pthread_setname_np(pthread_self(), "hypertracer");
#elif defined(__APPLE__)
    pthread_setname_np("hypertracer");
#endif

    std::optional<ht::internal::file::TempFile> file;
    try {
        file.emplace(prefix, suffix, buffer_size);
    } catch (...) {
        info.set_exception(std::current_exception());
        error.set_value();
        while (chan_reader.read().has_value()) {
        }
        return;
    }
    info.set_value(FileInfo{
        .filename = file->get_filename(),
        .fd = file->get_fd(),
    });
    try {
        file->write({std::byte(0x9f)});
        bool has_stalled = false;
        for (;;) {
            if (!has_stalled) {
                if (chan_reader.get_stall_count() != 0) {
                    has_stalled = true;
                    ht::internal::warning("warning: ht::Tracer::Internal::thread_main: tracing cannot keep up. try increasing tracing buffer size through HYPERTRACER_THREAD_BUFFER or file buffer size through HYPERTRACER_FILE_BUFFER\n"sv);
                }
            }
            auto msg = chan_reader.read();
            if (!msg.has_value()) {
                break;
            }
            msg->write_to(*file);
        }
    } catch (...) {
        file.reset();
        error.set_exception(std::current_exception());
        while (chan_reader.read().has_value()) {
        }
        return;
    }
    try {
        file->write({std::byte(0xff)});
    } catch (...) {
        error.set_exception(std::current_exception());
        return;
    }
    file.reset();
    error.set_value();
}

ht::internal::msg::Duration Tracer::Internal::get_timediff() {
    struct timespec real;
    if (::clock_gettime(CLOCK_REALTIME, &real) != 0) {
        throw std::system_error(errno, std::generic_category(), "clock_gettime(CLOCK_REALTIME)");
    }
    struct timespec mono;
    if (::clock_gettime(CLOCK_MONOTONIC, &mono) != 0) {
        throw std::system_error(errno, std::generic_category(), "clock_gettime(CLOCK_MONOTONIC)");
    }
    return ht::internal::msg::Timestamp(real) - ht::internal::msg::Timestamp(mono);
}

ht::internal::msg::Timestamp Tracer::Internal::now() const {
    struct timespec mono;
    if (::clock_gettime(CLOCK_MONOTONIC, &mono) != 0) {
        throw std::system_error(errno, std::generic_category(), "clock_gettime(CLOCK_MONOTONIC)");
    }
    return ht::internal::msg::Timestamp(mono) + timediff;
}

} // namespace ht
