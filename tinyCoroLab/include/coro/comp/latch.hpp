#pragma once

#include <atomic>
#include <coroutine>
#include <vector>

#include "coro/detail/types.hpp"
#include "coro/scheduler.hpp"
#include "coro/spinlock.hpp"
namespace coro
{

class latch
{
    struct awaiter
    {
        latch& m_latch;

        bool await_ready() noexcept
        {
            // 计数已经是 0，直接不挂起
            return m_latch.m_count.load(std::memory_order_acquire) == 0;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            std::lock_guard<detail::spinlock> lock(m_latch.m_lock);
            // 二次检查：防止 await_ready 到 await_suspend 之间 count_down 到 0
            if (m_latch.m_count.load(std::memory_order_acquire) == 0)
            {
                submit_to_scheduler(handle);
                return;
            }
            m_latch.m_waiters.push_back(handle);
        }

        void await_resume() noexcept {}
    };

public:
    latch(std::uint64_t count) noexcept : m_count(count) {}
    latch(const latch&)                    = delete;
    latch(latch&&)                         = delete;
    auto operator=(const latch&) -> latch& = delete;
    auto operator=(latch&&) -> latch&      = delete;

    auto count_down() noexcept -> void
    {
        // 先减 1，再检查是否到 0
        auto prev = m_count.fetch_sub(1, std::memory_order_acq_rel);
        if (prev == 1)  // 减之前是 1，减完变成 0
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
    std::atomic<uint64_t>                m_count;
    detail::spinlock                     m_lock;
    std::vector<std::coroutine_handle<>> m_waiters;
};

class latch_guard
{
public:
    latch_guard(latch& l) noexcept : m_l(l) {}
    ~latch_guard() noexcept { m_l.count_down(); }

private:
    latch& m_l;
};

}; // namespace coro
