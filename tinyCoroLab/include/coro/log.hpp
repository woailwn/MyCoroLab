#pragma once
#include <iostream>
#include <memory>
#include <string>

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

#include "config.h"

namespace coro::log
{
#define CONFIG_LOG_LEVEL(log_level) spdlog::level::log_level

using std::make_shared;
using std::shared_ptr;
using std::string;
using spdlogger = shared_ptr<spdlog::logger>;

class logger
{
public:
#ifdef LOGTOFILE
    static auto get_logger() noexcept -> spdlogger&
    {
        static logger log;
        return log.m_logger;
    };
#endif

    logger(const logger&)                    = delete;
    logger(logger&&)                         = delete;
    auto operator=(const logger&) -> logger& = delete;
    auto operator=(logger&&) -> logger&      = delete;

private:
    logger() noexcept
    {
#ifdef LOGTOFILE
        string log_path = string(SOURCE_DIR) + string(coro::config::kLogFileName);
        m_logger        = spdlog::create<spdlog::sinks::basic_file_sink_mt>("corolog", log_path.c_str(), true);
        m_logger->set_pattern("[%n][%Y-%m-%d %H:%M:%S.%e] [%l] [%t]  %v");
        m_logger->set_level(CONFIG_LOG_LEVEL(LOG_LEVEL));
        spdlog::flush_every(std::chrono::seconds(config::kFlushDura));
#endif
    }

    ~logger() noexcept
    {
#ifdef LOGTOFILE
        m_logger->flush_on(CONFIG_LOG_LEVEL(LOG_LEVEL));
#endif
    }

private:
#ifdef LOGTOFILE
    spdlogger m_logger;
#endif
};

template<typename... T>
inline void trace(const char* __restrict__ fmt, const T&... args)
{
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::trace)
    {
#ifdef LOGTOFILE
        logger::get_logger()->trace(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
        spdlog::trace(spdlog::fmt_runtime_string<char>{fmt}, args...);
    }
}

template<typename... T>
inline void debug(const char* __restrict__ fmt, const T&... args)
{
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::debug)
    {
#ifdef LOGTOFILE
        logger::get_logger()->debug(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
        spdlog::debug(spdlog::fmt_runtime_string<char>{fmt}, args...);
    }
}

template<typename... T>
inline void info(const char* __restrict__ fmt, const T&... args)
{
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::info)
    {
#ifdef LOGTOFILE
        logger::get_logger()->info(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
        spdlog::info(spdlog::fmt_runtime_string<char>{fmt}, args...);
    }
}

template<typename... T>
inline void warn(const char* __restrict__ fmt, const T&... args)
{
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::warn)
    {
#ifdef LOGTOFILE
        logger::get_logger()->warn(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
        spdlog::warn(spdlog::fmt_runtime_string<char>{fmt}, args...);
    }
}

template<typename... T>
inline void error(const char* __restrict__ fmt, const T&... args)
{
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::err)
    {
#ifdef LOGTOFILE
        logger::get_logger()->error(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
        spdlog::error(spdlog::fmt_runtime_string<char>{fmt}, args...);
    }
}

template<typename... T>
inline void critical(const char* __restrict__ fmt, const T&... args)
{
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::critical)
    {
#ifdef LOGTOFILE
        logger::get_logger()->critical(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
        spdlog::critical(spdlog::fmt_runtime_string<char>{fmt}, args...);
    }
}

}; // namespace coro::log