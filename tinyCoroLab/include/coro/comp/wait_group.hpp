/**
 * @file wait_group.hpp
 * @author JiahuiWang
 * @brief lab4c
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <atomic>
#include <coroutine>
#include <vector>

#include "coro/detail/types.hpp"
#include "coro/scheduler.hpp"
#include "coro/spinlock.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab4c, in this part you will build the basic coroutine
 * synchronization component��wait_group by modifing wait_group.hpp and wait_group.cpp.
 * Please ensure you have read the document of lab4c.
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

class wait_group
{
    struct awaiter
    {
        wait_group& m_wg;

        bool await_ready() noexcept
        {
            return m_wg.m_count.load(std::memory_order_acquire) == 0;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            std::lock_guard<detail::spinlock> lock(m_wg.m_lock);
            if (m_wg.m_count.load(std::memory_order_acquire) == 0)
            {
                submit_to_scheduler(handle);
                return;
            }
            m_wg.m_waiters.push_back(handle);
        }

        void await_resume() noexcept {}
    };

public:
    explicit wait_group(int count = 0) noexcept : m_count(count) {}
    wait_group(const wait_group&)                    = delete;
    wait_group(wait_group&&)                         = delete;
    auto operator=(const wait_group&) -> wait_group& = delete;
    auto operator=(wait_group&&) -> wait_group&      = delete;

    auto add(int count) noexcept -> void
    {
        m_count.fetch_add(count, std::memory_order_acq_rel);
    }

    auto done() noexcept -> void
    {
        auto prev = m_count.fetch_sub(1, std::memory_order_acq_rel);
        if (prev == 1)
        {
            std::lock_guard<detail::spinlock> lock(m_lock);
            for (auto handle : m_waiters)
            {
                submit_to_scheduler(handle);
            }
            m_waiters.clear();
        }
    }

    auto wait() noexcept -> awaiter { return awaiter{*this}; }

private:
    std::atomic<int>                     m_count;
    detail::spinlock                     m_lock;
    std::vector<std::coroutine_handle<>> m_waiters;
};

}; // namespace coro
