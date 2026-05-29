#pragma once

#include <exception>
#include <variant>

#include "coro/concepts/common.hpp"

namespace coro::detail
{
/**
 * @brief container is used to store variable and return
 *
 * @tparam T
 */
template<typename T>
// requires(!std::same_as<T, void> && !concepts::pod_type<T>)
struct container
{
private:
    struct unset_return_value
    {
        unset_return_value() {}
        unset_return_value(unset_return_value&&)      = delete;
        unset_return_value(const unset_return_value&) = delete;
        auto operator=(unset_return_value&&)          = delete;
        auto operator=(const unset_return_value&)     = delete;
    };

public:
    static constexpr bool return_type_is_reference = std::is_reference_v<T>;
    using stored_type =
        std::conditional_t<return_type_is_reference, std::remove_reference_t<T>*, std::remove_const_t<T>>;
    using variant_type = std::variant<unset_return_value, stored_type, std::exception_ptr>;

    template<typename value_type>
        requires(return_type_is_reference and std::is_constructible_v<T, value_type &&>) or
                (not return_type_is_reference and std::is_constructible_v<stored_type, value_type &&>)
    auto return_value(value_type&& value) -> void
    {
        if constexpr (return_type_is_reference)
        {
            T ref = static_cast<value_type&&>(value);
            m_storage.template emplace<stored_type>(std::addressof(ref));
        }
        else
        {
            m_storage.template emplace<stored_type>(std::forward<value_type>(value));
        }
    }

    auto return_value(stored_type&& value) -> void
        requires(not return_type_is_reference)
    {
        if constexpr (std::is_move_constructible_v<stored_type>)
        {
            m_storage.template emplace<stored_type>(std::move(value));
        }
        else
        {
            m_storage.template emplace<stored_type>(value);
        }
    }

    auto result() & -> decltype(auto)
    {
        if (std::holds_alternative<stored_type>(m_storage))
        {
            if constexpr (return_type_is_reference)
            {
                return static_cast<T>(*std::get<stored_type>(m_storage));
            }
            else
            {
                return static_cast<const T&>(std::get<stored_type>(m_storage));
            }
        }
        else if (std::holds_alternative<std::exception_ptr>(m_storage))
        {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        }
        else
        {
            throw std::runtime_error{"The return value was never set, did you execute the coroutine?"};
        }
    }

    auto result() const& -> decltype(auto)
    {
        if (std::holds_alternative<stored_type>(m_storage))
        {
            if constexpr (return_type_is_reference)
            {
                return static_cast<std::add_const_t<T>>(*std::get<stored_type>(m_storage));
            }
            else
            {
                return static_cast<const T&>(std::get<stored_type>(m_storage));
            }
        }
        else if (std::holds_alternative<std::exception_ptr>(m_storage))
        {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        }
        else
        {
            throw std::runtime_error{"The return value was never set, did you execute the coroutine?"};
        }
    }

    auto result() && -> decltype(auto)
    {
        if (std::holds_alternative<stored_type>(m_storage))
        {
            if constexpr (return_type_is_reference)
            {
                return static_cast<T>(*std::get<stored_type>(m_storage));
            }
            else if constexpr (std::is_move_constructible_v<T>)
            {
                return static_cast<T&&>(std::get<stored_type>(m_storage));
            }
            else
            {
                return static_cast<const T&&>(std::get<stored_type>(m_storage));
            }
        }
        else if (std::holds_alternative<std::exception_ptr>(m_storage))
        {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        }
        else
        {
            throw std::runtime_error{"The return value was never set, did you execute the coroutine?"};
        }
    }

    auto set_exception() noexcept -> void { new (&m_storage) variant_type(std::current_exception()); }

    inline auto value_ready() noexcept -> bool { return std::holds_alternative<stored_type>(m_storage); }

    inline auto value_exception() noexcept -> bool { return std::holds_alternative<std::exception_ptr>(m_storage); }

    inline auto value_unset() noexcept -> bool { return !value_ready() && !value_exception(); }

private:
    variant_type m_storage{};
};

template<concepts::pod_type T>
struct container<T>
{
public:
    container() noexcept : m_state(value_state::none) {}
    ~container() noexcept
    {
        if (m_state == value_state::exception)
        {
            m_exception_ptr.~exception_ptr();
        }
    };

    void return_value(T value) noexcept
    {
        m_value = value;
        m_state = value_state::value;
    }

    T result() noexcept
    {
        if (m_state == value_state::value)
        {
            return m_value;
        }
        else if (m_state == value_state::exception)
        {
            std::rethrow_exception(m_exception_ptr);
        }
        else
        {
            // throw std::runtime_error{"The return value was never set, did you execute the coroutine?"};
            return T{};
        }
    }

    auto set_exception() noexcept -> void
    {
        m_exception_ptr = std::current_exception();
        m_state         = value_state::exception;
    }

    inline auto value_ready() noexcept -> bool { return m_state == value_state::value; }

    inline auto value_exception() noexcept -> bool { return m_state == value_state::exception; }

    inline auto value_unset() noexcept -> bool { return m_state == value_state::none; }

private:
    union
    {
        T                  m_value;
        std::exception_ptr m_exception_ptr;
    };
    enum class value_state : uint8_t
    {
        none,
        value,
        exception
    } m_state;
};

}; // namespace coro::detail
