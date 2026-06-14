#pragma once

#include <chrono>
#ifndef _SYS_TIME_H
    #define _SYS_TIME_H
    #include <sys/time.h>
#endif

#include "coro/attribute.hpp"
#include "coro/concepts/common.hpp"
#include "coro/io/base_awaiter.hpp"

// 待办: Add time bias: timeout_bias_nanosecond
namespace coro::time
{

using coro_system_clock          = std::chrono::system_clock;
using coro_steady_clock          = std::chrono::steady_clock;
using coro_high_resolution_clock = std::chrono::high_resolution_clock;

#define timeout_abs           IORING_TIMEOUT_ABS
#define timeout_boottime      IORING_TIMEOUT_BOOTTIME
#define timeout_realtime      IORING_TIMEOUT_REALTIME
#define timeout_monotonic     0
#define timeout_etime_success IORING_TIMEOUT_ETIME_SUCCESS
#define timeout_multishot     IORING_TIMEOUT_MULTISHOT

namespace detail
{
template<typename Rep, typename Period>
inline auto get_kernel_timespec(std::chrono::duration<Rep, Period> time_duration) -> __kernel_timespec
{
    auto seconds     = std::chrono::duration_cast<std::chrono::seconds>(time_duration);
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(time_duration - seconds);
    return __kernel_timespec{.tv_sec = seconds, .tv_nsec = nanoseconds};
}

template<typename Clock, typename Duration>
requires(coro::concepts::in_types<
         Clock,
         coro_system_clock,
         coro_steady_clock,
         coro_high_resolution_clock>) inline auto get_kernel_timespec(std::chrono::time_point<Clock, Duration>
                                                                          time_point) -> __kernel_timespec
{
    return get_kernel_timespec(time_point.time_since_epoch());
}

}; // namespace detail

using ::coro::detail::local_engine;
using coro::io::detail::io_info;
using coro::io::detail::io_type;

class timer
{
    struct timer_awaiter : coro::io::detail::base_io_awaiter
    {
        timer_awaiter(__kernel_timespec ts, int count, unsigned flags) noexcept
        {
            m_info.type = io_type::timer;
            m_info.cb   = &timer_awaiter::callback;
            m_ts        = ts;

            io_uring_prep_timeout(m_urs, &m_ts, count, flags);
            io_uring_sqe_set_data(m_urs, &m_info);
            local_engine().add_io_submit();
        }
        static auto callback(io_info* data, int res) noexcept -> void
        {
            // ignore timeout error
            if (res == -ETIME)
            {
                res = 0;
            }
            data->result = res;
            submit_to_context(data->handle);
        }

        // avoid race conddition, so copy ts
        __kernel_timespec m_ts;
    };

public:
    explicit timer(unsigned flag = timeout_monotonic) noexcept : m_flag(flag) {}

        CORO_INLINE auto add_seconds(uint64_t seconds) -> timer&
    {
        m_ts.tv_sec += seconds;
        return *this;
    }

        CORO_INLINE auto add_mseconds(uint64_t mseconds) -> timer&
    {
        m_ts.tv_nsec += (1000000 * mseconds);
        return *this;
    }

        CORO_INLINE auto add_useconds(uint64_t useconds) -> timer&
    {
        m_ts.tv_nsec += (1000 * useconds);
        return *this;
    }

        CORO_INLINE auto add_nseconds(uint64_t nseconds) -> timer&
    {
        m_ts.tv_nsec += nseconds;
        return *this;
    }

    template<typename Rep, typename Period>
    auto set_by_duration(std::chrono::duration<Rep, Period> time_duration) -> timer&
    {
        m_ts = detail::get_kernel_timespec(time_duration);
        return *this;
    }

        template<typename Clock, typename Duration>
    requires(coro::concepts::in_types<
             Clock,
             coro_system_clock,
             coro_steady_clock,
             coro_high_resolution_clock>) auto set_by_timepoint(std::chrono::time_point<Clock, Duration> time_point)
        -> timer&
    {
        m_ts = detail::get_kernel_timespec(time_point);
        return *this;
    }

    auto operator co_await() noexcept
    {
        // set count to 1 means we just want to produce 1 cqe
        return timer_awaiter(m_ts, 1, 0);
    }

private:
    __kernel_timespec m_ts;
    unsigned          m_flag{0};
};

}; // namespace coro::time
