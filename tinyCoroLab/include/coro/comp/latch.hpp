/**
 * @file latch.hpp
 * @author JiahuiWang
 * @brief lab4b
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <atomic>

#include "coro/detail/types.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab4b, in this part you will build the basic coroutine
 * synchronization component - latch by modifing latch.hpp and latch.cpp. Please ensure
 * you have read the document of lab4b.
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

// TODO[lab4b]: This latch is an example to make complie success,
// You should delete it and add your implementation, I don't care what you do,
// but keep the function count_down() and wait()'s declaration same with example.
class latch
{
public:
    latch(std::uint64_t count) noexcept {}
    latch(const latch&)                    = delete;
    latch(latch&&)                         = delete;
    auto operator=(const latch&) -> latch& = delete;
    auto operator=(latch&&) -> latch&      = delete;

    auto count_down() noexcept -> void {}

    auto wait() noexcept -> detail::noop_awaiter { return {}; }
};

/**
 * @brief RAII for latch
 *
 */
class latch_guard
{
public:
    latch_guard(latch& l) noexcept : m_l(l) {}
    ~latch_guard() noexcept { m_l.count_down(); }

private:
    latch& m_l;
};

}; // namespace coro
