#pragma once

#include <concepts>
#include <coroutine>
#include <type_traits>
#include <utility>

namespace coro::concepts
{
template<typename type, typename... types>
concept in_types = (std::same_as<type, types> || ...);

template<typename type>
concept list_type = requires(type t) {
    { t.next() } -> std::same_as<type*>;
};

template<typename T>
concept pod_type = std::is_standard_layout_v<T> && std::is_trivial_v<T>;

template<typename T>
concept void_pod_type = pod_type<T> || std::is_void_v<T>;

template<typename type, typename... types>
concept all_same_type = (std::same_as<type, types> || ...);

template<typename... types>
concept all_void_type = (std::is_void_v<types> || ...);

template<typename... types>
concept all_noref_pod = (pod_type<types> || ...);

template<typename T>
concept conventional_type = !std::is_void_v<T> && !std::is_reference_v<T> && !std::is_const_v<T>;

}; // namespace coro::concepts