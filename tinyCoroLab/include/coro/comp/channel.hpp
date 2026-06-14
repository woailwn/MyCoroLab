#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <coroutine>
#include <cstddef>
#include <optional>
#include <queue>

#include "coro/comp/condition_variable.hpp"
#include "coro/comp/mutex.hpp"
#include "coro/concepts/common.hpp"
#include "coro/context.hpp"
#include "coro/spinlock.hpp"
#include "coro/task.hpp"

namespace coro
{

namespace detail
{
}; // namespace detail

template<concepts::conventional_type T, size_t capacity = 1>
class channel
{
    using data_type = std::optional<T>;

    struct send_awaiter;
    struct recv_awaiter;

    struct pending_sender
    {
        std::coroutine_handle<> handle;
        send_awaiter*           aw;
    };

    struct pending_receiver
    {
        std::coroutine_handle<> handle;
        recv_awaiter*           aw;
    };

    detail::spinlock             m_lock;
    std::queue<T>                m_buffer;
    bool                         m_closed{false};
    std::queue<pending_sender>   m_senders;
    std::queue<pending_receiver> m_receivers;

    bool try_send_locked(T& value, bool& result) noexcept
    {
        if (m_closed)
        {
            return true;  // m_result defaults to false
        }
        if (!m_receivers.empty())
        {
            auto [handle, aw] = m_receivers.front();
            m_receivers.pop();
            aw->m_result = std::move(value);
            local_context().submit_task(handle);
            result = true;
            return true;
        }
        if (m_buffer.size() < capacity)
        {
            m_buffer.push(std::move(value));
            result = true;
            return true;
        }
        return false;
    }

    bool try_recv_locked(data_type& result) noexcept
    {
        if (!m_buffer.empty())
        {
            result = std::move(m_buffer.front());
            m_buffer.pop();
            if (!m_senders.empty())
            {
                auto [handle, aw] = m_senders.front();
                m_senders.pop();
                m_buffer.push(std::move(aw->m_value));
                aw->m_result = true;
                local_context().submit_task(handle);
            }
            return true;
        }
        if (m_closed)
        {
            result = std::nullopt;
            return true;
        }
        return false;
    }

    struct send_awaiter
    {
        channel& m_ch;
        T        m_value;
        bool     m_result{false};

        template<typename value_type>
        send_awaiter(channel& ch, value_type&& val) noexcept
            : m_ch(ch), m_value(std::forward<value_type>(val))
        {}

        bool await_ready() noexcept
        {
            std::lock_guard<detail::spinlock> lock(m_ch.m_lock);
            return m_ch.try_send_locked(m_value, m_result);
        }

        // Returns false = resume immediately; true = do suspend
        bool await_suspend(std::coroutine_handle<> handle) noexcept
        {
            std::lock_guard<detail::spinlock> lock(m_ch.m_lock);
            if (m_ch.try_send_locked(m_value, m_result))
                return false;
            m_ch.m_senders.push({handle, this});
            return true;
        }

        bool await_resume() noexcept { return m_result; }
    };

    struct recv_awaiter
    {
        channel&  m_ch;
        data_type m_result;

        bool await_ready() noexcept
        {
            std::lock_guard<detail::spinlock> lock(m_ch.m_lock);
            return m_ch.try_recv_locked(m_result);
        }

        bool await_suspend(std::coroutine_handle<> handle) noexcept
        {
            std::lock_guard<detail::spinlock> lock(m_ch.m_lock);
            if (m_ch.try_recv_locked(m_result))
                return false;
            m_ch.m_receivers.push({handle, this});
            return true;
        }

        data_type await_resume() noexcept { return std::move(m_result); }
    };

public:
    template<typename value_type>
        requires(std::is_constructible_v<T, value_type&&>)
    auto send(value_type&& value) noexcept -> send_awaiter
    {
        return send_awaiter(*this, std::forward<value_type>(value));
    }

    auto recv() noexcept -> recv_awaiter
    {
        return recv_awaiter{*this};
    }

    auto close() noexcept -> void
    {
        std::lock_guard<detail::spinlock> lock(m_lock);
        m_closed = true;
        while (!m_senders.empty())
        {
            auto [handle, aw] = m_senders.front();
            m_senders.pop();
            aw->m_result = false;
            local_context().submit_task(handle);
        }
        while (!m_receivers.empty())
        {
            auto [handle, aw] = m_receivers.front();
            m_receivers.pop();
            aw->m_result = std::nullopt;
            local_context().submit_task(handle);
        }
    }
};

}; // namespace coro
