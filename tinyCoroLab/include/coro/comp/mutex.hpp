/**
 * @file mutex.hpp
 * @author JiahuiWang
 * @brief lab4d
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <atomic>
#include <cassert>
#include <coroutine>
#include <type_traits>

#include "coro/comp/mutex_guard.hpp"
#include "coro/detail/types.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab4d, in this part you will build the basic coroutine
 * synchronization component----mutex by modifing mutex.hpp and mutex.cpp.
 * Please ensure you have read the document of lab4d.
 *
 * @warning You should carefully consider whether each implementation should be thread-safe.
 *
 * You should follow the rules below in this part:
 *
 * @note The location marked by todo is where you must add code, but you can also add code anywhere
 * you want, such as function and class definitions, even member variables.
 *
 * @note lab4 and lab5 are free designed lab, leave the interfaces that the test case will use,
 * and then, enjoy yourself!
 */

class context;

// TODO[lab4d]: This mutex is an example to make complie success,
// You should delete it and add your implementation, I don't care what you do,
// but keep the member function and construct function's declaration same with example.
class mutex
{
    // Just make lock_guard() compile success
    struct guard_awaiter : detail::noop_awaiter
    {
        guard_awaiter(mutex& m) noexcept : mtx(m) {}
        auto   await_resume() -> detail::lock_guard<mutex> { return detail::lock_guard<mutex>(mtx); }
        mutex& mtx;
    };

public:
    mutex() noexcept {}
    ~mutex() noexcept {}

    auto try_lock() noexcept -> bool { return {}; }

    auto lock() noexcept -> detail::noop_awaiter { return {}; };

    auto unlock() noexcept -> void {};

    auto lock_guard() noexcept -> guard_awaiter { return {*this}; };
};

}; // namespace coro
