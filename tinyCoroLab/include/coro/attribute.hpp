#pragma once

#include "config.h"

// This is a GCC extension; define it only for GCC and compilers that emulate GCC.
#if defined(__GNUC__) && !defined(__clang__)
    #define CORO_ATTR(attr) __attribute__((attr))
    #define CORO_INLINE     __attribute__((always_inline))
#else
    #define CORO_ATTR(attr)
    #define CORO_INLINE
#endif

#define CORO_AWAIT_HINT   nodiscard("Did you forget to co_await?")
#define CORO_DISCARD_HINT nodiscard("Discard is improper")
#define CORO_ALIGN        alignas(::coro::config::kCacheLineSize)
#define CORO_MAYBE_UNUSED maybe_unused

// if one function is marked by CORO_TEST_USED, this means test
// will use this function, don't modify this function declaration
#define CORO_TEST_USED(...)

#define CORO_NO_COPY_MOVE(classname)                                                                                   \
    classname(const classname&)            = delete;                                                                   \
    classname(classname&&)                 = delete;                                                                   \
    classname& operator=(const classname&) = delete;                                                                   \
    classname& operator=(classname&&)      = delete

// #define CORO_TEST_USED
