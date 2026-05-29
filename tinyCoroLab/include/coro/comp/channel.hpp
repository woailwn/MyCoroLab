/**
 * @file channel.hpp
 * @author JiahuiWang
 * @brief lab5c
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <optional>

#include "coro/comp/condition_variable.hpp"
#include "coro/comp/mutex.hpp"
#include "coro/concepts/common.hpp"
#include "coro/task.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab5c, in this part you will build the basic coroutine
 * synchronization component¡ª¡ªchannel by modifing channel.hpp and channel.cpp.
 * Please ensure you have read the document of lab5c.
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
// TODO[lab5c]: Add code that you don't want to use externally in namespace detail
}; // namespace detail

// TODO[lab5c]: This channel is an example to make complie success,
// You should delete it and add your implementation, I don't care what you do,
// but keep the member function and construct function's declaration same with example.
template<concepts::conventional_type T, size_t capacity = 1>
class channel
{
    using data_type = std::optional<T>;

public:
    template<typename value_type>
        requires(std::is_constructible_v<T, value_type &&>)
    auto send(value_type&& value) noexcept -> task<bool>
    {
        co_return {};
    }

    auto recv() noexcept -> task<data_type> { co_return {}; }

    auto close() noexcept -> void {}
};

}; // namespace coro
