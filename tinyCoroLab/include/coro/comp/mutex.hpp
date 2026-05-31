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
#include <queue>
#include <type_traits>

#include "coro/comp/mutex_guard.hpp"
#include "coro/context.hpp"
#include "coro/detail/types.hpp"
#include "coro/scheduler.hpp"
#include "coro/spinlock.hpp"

namespace coro
{

class context;

class mutex
{
    // lock() 的 awaiter，await_resume 返回 void
    struct lock_awaiter
    {
        mutex& m_mutex;

        bool await_ready() noexcept
        {
            return m_mutex.try_lock();
        }
        
        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            std::lock_guard<detail::spinlock> lock(m_mutex.m_lock);
            // 持有 spinlock 后再尝试抢锁，spinlock 保证和 unlock 互斥
            bool expected = false;
            if (m_mutex.m_locked.compare_exchange_strong(expected, true, std::memory_order_acquire))
            {
                local_context().submit_task(handle);
                return;
            }
            m_mutex.m_waiters.push(handle);
        }

        void await_resume() noexcept {}
    };

    // lock_guard() 的 awaiter，await_resume 返回 lock_guard
    struct guard_awaiter
    {
        mutex& m_mutex;

        bool await_ready() noexcept
        {
            return m_mutex.try_lock();
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            std::lock_guard<detail::spinlock> lock(m_mutex.m_lock);
            bool expected = false;
            if (m_mutex.m_locked.compare_exchange_strong(expected, true, std::memory_order_acquire))
            {
                local_context().submit_task(handle);
                return;
            }
            m_mutex.m_waiters.push(handle);
        }

        auto await_resume() noexcept -> detail::lock_guard<mutex>
        {
            return detail::lock_guard<mutex>(m_mutex);
        }
    };

public:
    mutex() noexcept {}
    ~mutex() noexcept {}

    auto try_lock() noexcept -> bool
    {
        bool expected = false;
        return m_locked.compare_exchange_strong(expected, true, std::memory_order_acquire);
    }

    auto lock() noexcept -> lock_awaiter { return lock_awaiter{*this}; }

    auto unlock() noexcept -> void
    {
        std::lock_guard<detail::spinlock> lock(m_lock);
        if (!m_waiters.empty())
        {
            // 把锁直接转交给下一个等待者，不改变 m_locked 状态
            // 用 local_context().submit_task 绕过 register_wait（handle 已被计数）
            auto handle = m_waiters.front();
            m_waiters.pop();
            local_context().submit_task(handle);
        }
        else
        {
            // 没有等待者，释放锁
            m_locked.store(false, std::memory_order_release);
        }
    }

    auto lock_guard() noexcept -> guard_awaiter { return guard_awaiter{*this}; }

    // for condition_variable: try lock, if fail enqueue handle into mutex waiters
    auto enqueue_waiter(std::coroutine_handle<> handle) noexcept -> void
    {
        std::lock_guard<detail::spinlock> lock(m_lock);
        bool expected = false;
        if (m_locked.compare_exchange_strong(expected, true, std::memory_order_acquire))
        {
            local_context().submit_task(handle);
        }
        else
        {
            m_waiters.push(handle);
        }
    }

private:
    std::atomic<bool>                    m_locked{false};
    detail::spinlock                     m_lock;
    std::queue<std::coroutine_handle<>>  m_waiters;
};

}; // namespace coro
