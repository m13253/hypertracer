#include "file"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <stdexcept>
#include <stdlib.h>
#include <system_error>
#include <time.h>
#include <unistd.h>

namespace ht {
namespace internal {
namespace file {

TempFile::TempFile(std::string_view prefix, std::string_view suffix, std::size_t buffer_size) :
    buf_cap(buffer_size) {
    assert(buffer_size != 0);
    if (prefix.find('\0') != std::string_view::npos || suffix.find('\0') != std::string_view::npos) {
        throw std::invalid_argument("filename contains NUL byte");
    }

    time_t timer = ::time(nullptr);
    if (timer == time_t(-1)) {
        throw std::system_error(errno, std::generic_category(), "time");
    }
    struct tm tm;
    struct tm *ptm = ::gmtime_r(&timer, &tm);
    if (ptm == nullptr) {
        throw std::system_error(errno, std::generic_category(), "gmtime_r");
    }
    std::array<char, 21> date_buffer;
    std::size_t date_len = ::strftime(date_buffer.data(), date_buffer.size(), "%Y-%m-%dT%H-%M-%SZ", ptm);

    filename.reserve(prefix.size() + suffix.size() + 28);
    filename.append(prefix);
    filename.push_back('_');
    filename.append(date_buffer.data(), date_len);
    filename.push_back('_');
    filename.append(6, 'X');
    filename.append(suffix);

    fd = ::mkostemps(filename.data(), suffix.size(), O_CLOEXEC);
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "mkostemps");
    }
    buf = std::make_unique_for_overwrite<std::byte[]>(buffer_size);
}

TempFile::~TempFile() noexcept(false) {
    if (fail) {
        ::close(fd);
        return;
    }
    try {
        do_flush();
    } catch (...) {
        ::close(fd);
        throw;
    }
    if (::close(fd) != 0) {
        throw std::system_error(errno, std::generic_category(), "close");
    }
}

std::size_t TempFile::do_flush() {
    std::size_t bytes_written = 0;
    while (buf_front != buf_back) {
        auto bytes_count = ::write(fd, &buf[buf_front], buf_back - buf_front);
        if (bytes_count == -1) {
            fail = true;
            throw std::system_error(errno, std::generic_category(), "write");
        }
        buf_front += std::size_t(bytes_count);
        bytes_written += std::size_t(bytes_count);
    }
    buf_front = 0;
    buf_back = 0;
    return bytes_written;
}

std::size_t TempFile::write(std::span<const std::byte> data) {
    if (fail) {
        std::terminate();
    }
    std::size_t bytes_written = 0;
    while (!data.empty()) {
        if (data.size() >= buf_cap - buf_back) {
            // We need to flush at least once
            if (buf_front == buf_back) {
                // The buffer is empty
                auto bytes_count = ::write(fd, data.data(), buf_cap - buf_back);
                if (bytes_count == -1) {
                    fail = true;
                    throw std::system_error(errno, std::generic_category(), "write");
                }
                bytes_written += std::size_t(bytes_count);
                data = data.subspan(std::size_t(bytes_count));
            } else {
                // Append the data to the buffer, then flush
                std::size_t bytes_count = buf_cap - buf_back;
                std::copy_n(data.data(), bytes_count, &buf[buf_back]);
                buf_back = buf_cap;
                bytes_written += do_flush();
                data = data.subspan(bytes_count);
            }
        } else {
            // We don't need to flush
            std::size_t bytes_count = data.size();
            std::copy_n(data.data(), bytes_count, &buf[buf_back]);
            buf_back += bytes_count;
            break;
        }
    }
    return bytes_written;
}

} // namespace file
} // namespace internal
} // namespace ht
