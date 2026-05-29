/**
 * @file when_all.hpp
 * @author JiahuiWang
 * @brief lab5a
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <array>
#include <ranges>
#include <tuple>
#include <vector>

#include "coro/attribute.hpp"
#include "coro/comp/latch.hpp"
#include "coro/concepts/awaitable.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab5a, in this part you will build the basic coroutine
 * synchronization component - when_all by modifing when_all.hpp.
 * Please ensure syou have read the document of lab5a.
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
namespace detail
{
// TODO[lab5a]: Add code that you don't want to use externally in namespace detail
}; // namespace detail

// @warning: make compile success, don't use it
template<typename return_type>
struct awaiter : public detail::noop_awaiter
{
    auto await_resume() -> std::array<return_type, 1> { return {}; }
};

template<>
struct awaiter<void> : detail::noop_awaiter
{
};

template<>
struct awaiter<std::vector<int>> : public detail::noop_awaiter
{
    auto await_resume() -> std::vector<int> { return {}; }
};

template<concepts::awaitable... awaitables_type>
[[CORO_TEST_USED(lab5a)]] [[CORO_AWAIT_HINT]] static auto when_all(awaitables_type... awaitables) noexcept
    -> awaiter<int>
{
    // TODO[lab5a] : Add codes if you need,
    // change return type to awaiter implemented by you

    return {};
}

// TODO[lab5a] : You can change template parameters if you need
template<std::ranges::range range_type>
[[CORO_TEST_USED(lab5a)]] [[CORO_AWAIT_HINT]] static auto when_all(range_type&& awaitables) -> awaiter<std::vector<int>>
{
    // TODO[lab5a] : Add codes if you need,
    // change return type to awaiter implemented by you
    return {};
}
}; // namespace coro
