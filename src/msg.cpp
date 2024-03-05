#include "msg"

#define _GNU_SOURCE
#include <cstdint>
#include <unistd.h>
#ifndef __linux__
#include <pthread.h>
#endif

namespace ht {
namespace internal {
namespace msg {

void get_pid_tid() noexcept {
    last_pid = std::uint64_t(getpid());
#ifdef __linux__
    last_tid = std::uint64_t(gettid());
#elif defined(__APPLE__)
    pthread_threadid_np(nullptr, &last_tid);
#else
    last_tid = std::uint64_t(pthread_self());
#endif
}

thread_local std::uint64_t last_pid;
thread_local std::uint64_t last_tid;
thread_local std::uint64_t ID::last_mid = 0;
thread_local bool pid_tid_avail = false;

} // namespace msg
} // namespace internal
} // namespace ht
