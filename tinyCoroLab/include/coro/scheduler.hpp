#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "config.h"
#ifdef ENABLE_MEMORY_ALLOC
    #include "coro/allocator/memory.hpp"
#endif
#include "coro/dispatcher.hpp"

namespace coro
{

/**
 * @brief scheduler just control context to run and stop,
 * it also use dispatcher to decide which context can accept the task
 *
 */
class scheduler
{
    friend context;

public:
    [[CORO_TEST_USED(lab2b)]] inline static auto init(size_t ctx_cnt = std::thread::hardware_concurrency()) noexcept
        -> void
    {
        if (ctx_cnt == 0)
        {
            ctx_cnt = std::thread::hardware_concurrency();
        }
        get_instance()->init_impl(ctx_cnt);
    }

    /**
     * @brief loop work mode, auto wait all context finish job
     *
     */
    [[CORO_TEST_USED(lab2b)]] inline static auto loop() noexcept -> void { get_instance()->loop_impl(); }

    static inline auto submit(task<void>&& task) noexcept -> void
    {
        auto handle = task.handle();
        task.detach();
        submit(handle);
    }

    static inline auto submit(task<void>& task) noexcept -> void { submit(task.handle()); }

    [[CORO_TEST_USED(lab2b)]] inline static auto submit(std::coroutine_handle<> handle) noexcept -> void
    {
        get_instance()->submit_task_impl(handle);
    }

private:
    static auto get_instance() noexcept -> scheduler*
    {
        static scheduler sc;
        return &sc;
    }

    [[CORO_TEST_USED(lab2b)]] auto init_impl(size_t ctx_cnt) noexcept -> void;

    [[CORO_TEST_USED(lab2b)]] auto loop_impl() noexcept -> void;

    auto stop_impl() noexcept -> void;

    [[CORO_TEST_USED(lab2b)]] auto submit_task_impl(std::coroutine_handle<> handle) noexcept -> void;

    // TODO[lab2b]: Add more function if you need

private:
    size_t                                              m_ctx_cnt{0};
    detail::ctx_container                               m_ctxs;
    detail::dispatcher<coro::config::kDispatchStrategy> m_dispatcher;
    // TODO[lab2b]: Add more member variables if you need

#ifdef ENABLE_MEMORY_ALLOC
    // Memory Allocator
    coro::allocator::memory::memory_allocator<coro::config::kMemoryAllocator> m_mem_alloc;
#endif
};

inline void submit_to_scheduler(task<void>&& task) noexcept
{
    scheduler::submit(std::move(task));
}

inline void submit_to_scheduler(task<void>& task) noexcept
{
    scheduler::submit(task.handle());
}

inline void submit_to_scheduler(std::coroutine_handle<> handle) noexcept
{
    scheduler::submit(handle);
}

}; // namespace coro
