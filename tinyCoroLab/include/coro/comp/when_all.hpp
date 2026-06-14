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

namespace detail
{
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
[[CORO_AWAIT_HINT]] static auto when_all(awaitables_type... awaitables) noexcept
    -> awaiter<int>
{
    return {};
}

template<std::ranges::range range_type>
[[CORO_AWAIT_HINT]] static auto when_all(range_type&& awaitables) -> awaiter<std::vector<int>>
{
    return {};
}
}; // namespace coro
