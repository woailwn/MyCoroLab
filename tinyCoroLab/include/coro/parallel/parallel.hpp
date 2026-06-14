#pragma once

#include <functional>

#include "coro/comp/when_all.hpp"
#include "coro/concepts/awaitable.hpp"
#include "coro/concepts/function_traits.hpp"

namespace coro::parallel
{
namespace detail
{
template<typename type>
concept parallel_tasks_void_type =
    std::ranges::range<type> && ::coro::concepts::awaitable_void<std::ranges::range_value_t<type>>;

template<typename type>
concept parallel_tasks_novoid_type =
    std::ranges::range<type> && ::coro::concepts::awaitable<std::ranges::range_value_t<type>>;

template<typename type>
concept reduce_operator_input = std::ranges::range<type>;
}; // namespace detail

// notes: reduce func is used to collect result, like mapreduce

template<detail::parallel_tasks_void_type tasks_type>
auto parallel_func(tasks_type&& tasks) -> task<void>
{
    co_await when_all(std::forward<tasks_type>(tasks));
}

template<typename func_type>
requires(coro::concepts::function_traits<func_type>::arity == 1) auto make_parallel_reduce_func(func_type func)
{
    using std_func_type = std::function<typename coro::concepts::function_traits<func_type>::return_type(
        typename coro::concepts::function_traits<func_type>::template args<0>::type)>;
    return std_func_type(func);
}

template<
    detail::parallel_tasks_novoid_type tasks_type,
    detail::reduce_operator_input      input_type,
    typename return_type,
    typename task_return_type =
        typename ::coro::concepts::awaitable_traits<std::ranges::range_value_t<tasks_type>>::awaiter_return_type>
requires(std::is_same_v<task_return_type, std::ranges::range_value_t<input_type>>) auto parallel_func(
    tasks_type&& tasks, std::function<return_type(input_type)> reduce) -> task<return_type>
{
    auto result = co_await when_all(std::forward<tasks_type>(tasks));
    co_return reduce(result);
}

}; // namespace coro::parallel
