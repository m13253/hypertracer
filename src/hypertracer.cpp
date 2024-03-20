#include "hypertracer"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cerrno>
#include <cstddef>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <system_error>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <algorithm>
#include <atomic>
#include <sched.h>
#include <sys/sysinfo.h>
#if __GLIBC__ < 2 || __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h> // glibc < 2.30 didn't have gettid() in unistd.h
#endif
#endif

namespace ht {
namespace internal {

namespace compat {

std::timespec clock_gettime_monotonic() {
    struct timespec time;
    if (::clock_gettime(CLOCK_MONOTONIC, &time) != 0) {
        throw std::system_error(errno, std::generic_category(), "clock_gettime(CLOCK_MONOTONIC)");
    }
    return time;
}

std::timespec clock_gettime_realtime() {
    struct timespec time;
    if (::clock_gettime(CLOCK_REALTIME, &time) != 0) {
        throw std::system_error(errno, std::generic_category(), "clock_gettime(CLOCK_REALTIME)");
    }
    return time;
}

void close(int fd) {
    if (::close(fd) != 0) {
        throw std::system_error(errno, std::generic_category(), "close");
    }
}

int close_noexcept(int fd) noexcept {
    return ::close(fd);
}

std::uint64_t getpid() noexcept {
    thread_local bool pid_avail = false;
    thread_local std::uint64_t pid;
    if (!pid_avail) {
        pid_avail = true;
        pid = std::uint64_t(::getpid());
    }
    return pid;
}

std::uint64_t gettid() noexcept {
    thread_local bool tid_avail = false;
    thread_local std::uint64_t tid;
    if (!tid_avail) {
        tid_avail = true;
#ifdef __linux__
#if __GLIBC__ < 2 || __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
        tid = std::uint64_t(::syscall(SYS_gettid)); // glibc < 2.30 didn't have gettid() in unistd.h
#else
        tid = std::uint64_t(::gettid());
#endif
#elif defined(__APPLE__)
        ::pthread_threadid_np(nullptr, &tid);
#else
        tid = std::uint64_t(::pthread_self());
#endif
    }
    return tid;
}

std::tm gmtime(const std::time_t *timep) {
    struct tm tm;
    struct tm *ptm = ::gmtime_r(timep, &tm);
    if (ptm == nullptr) {
        throw std::system_error(errno, std::generic_category(), "gmtime_r");
    }
    return *ptm;
}

int mkostemps_cloexec(char *path, int slen) {
    int fd = ::mkostemps(path, slen, O_CLOEXEC);
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "mkostemps");
    }
    return fd;
}

void pthread_setaffinity_self_all() {
#ifdef __linux__
    static std::atomic_int num_cpus_cached = 0;
    int num_cpus = num_cpus_cached.load(std::memory_order::relaxed);
    if (num_cpus == 0) {
        // This API reads /sys/devices/system/cpu/possible
        num_cpus = ::get_nprocs_conf();
        num_cpus_cached.store(num_cpus, std::memory_order::relaxed);
    }
    std::size_t cpu_set_size = CPU_ALLOC_SIZE(num_cpus);
    cpu_set_t *cpu_set = CPU_ALLOC(num_cpus);
    std::fill_n(reinterpret_cast<std::byte *>(cpu_set), cpu_set_size, ~std::byte(0));
    int result = ::pthread_setaffinity_np(::pthread_self(), cpu_set_size, cpu_set);
    CPU_FREE(cpu_set);
    if (result != 0) {
        throw std::system_error(result, std::generic_category(), "pthread_setaffinity_np");
    }
#endif
}

void pthread_setname_self_noexcept(const char *name) noexcept {
#ifdef __linux__
    ::pthread_setname_np(::pthread_self(), name);
#elif defined(__APPLE__)
    ::pthread_setname_np(name);
#endif
}

std::size_t strftime(char *__restrict__ s, std::size_t max, const char *__restrict__ format, const std::tm *tm) noexcept {
    return ::strftime(s, max, format, tm);
}

std::time_t time(std::time_t *tloc) {
    std::time_t now = ::time(tloc);
    if (now == time_t(-1)) {
        throw std::system_error(errno, std::generic_category(), "time");
    }
    return now;
}

std::size_t write(int fd, const void *buf, std::size_t nbyte) {
    auto bytes_written = ::write(fd, buf, nbyte);
    if (bytes_written == -1) {
        throw std::system_error(errno, std::generic_category(), "write");
    }
    return std::size_t(bytes_written);
}

} // namespace compat

namespace msg {

thread_local std::uint64_t ID::last_mid = 0;

} // namespace msg

} // namespace internal
} // namespace ht
