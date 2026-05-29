/**
 * @file context.hpp
 * @author JiahuiWang
 * @brief lab2b
 * @version 1.1
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "config.h"
#include "coro/engine.hpp"
#include "coro/meta_info.hpp"
#include "coro/task.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab2b, in this part you will use engine to build context. Like
 * equipping soldiers with weapons, context will use engine to execute io task and computation task.
 *
 * @warning You should carefully consider whether each implementation should be thread-safe.
 *
 * You should follow the rules below in this part:
 *
 * @note Do not modify existing functions and class declaration, which may cause the test to not run
 * correctly, but you can change the implementation logic if you need.
 *
 * @note The location marked by todo is where you must add code, but you can also add code anywhere
 * you want, such as function and class definitions, even member variables.
 */

using config::ctx_id;
using std::atomic;
using std::jthread;
using std::make_unique;
using std::memory_order_acq_rel;
using std::memory_order_acquire;
using std::memory_order_relaxed;
using std::memory_order_release;
using std::stop_token;
using std::unique_ptr;

using detail::ginfo;
using detail::linfo;

using engine = detail::engine;

class scheduler;

/**
 * @brief Each context own one engine, it's the core part of tinycoro,
 * which can process computation task and io task
 *
 */
class context
{
    friend class scheduler;
public:
    context() noexcept;
    ~context() noexcept                = default;
    context(const context&)            = delete;
    context(context&&)                 = delete;
    context& operator=(const context&) = delete;
    context& operator=(context&&)      = delete;

    [[CORO_TEST_USED(lab2b)]] auto init() noexcept -> void;

    [[CORO_TEST_USED(lab2b)]] auto deinit() noexcept -> void;

    /**
     * @brief work thread start running
     *
     */
    [[CORO_TEST_USED(lab2b)]] auto start() noexcept -> void;

    /**
     * @brief send stop signal to work thread
     *
     */
    [[CORO_TEST_USED(lab2b)]] auto notify_stop() noexcept -> void;

    /**
     * @brief wait work thread stop
     *
     */
    inline auto join() noexcept -> void { m_job->join(); }

    inline auto submit_task(task<void>&& task) noexcept -> void
    {
        auto handle = task.handle();
        task.detach();
        this->submit_task(handle);
    }

    inline auto submit_task(task<void>& task) noexcept -> void { submit_task(task.handle()); }

    /**
     * @brief submit one task handle to context
     *
     * @param handle
     */
    [[CORO_TEST_USED(lab2b)]] auto submit_task(std::coroutine_handle<> handle) noexcept -> void;

    /**
     * @brief get context unique id
     *
     * @return ctx_id
     */
    inline auto get_ctx_id() noexcept -> ctx_id { return m_id; }

    /**
     * @brief add reference count of context
     *
     * @param register_cnt
     */
    [[CORO_TEST_USED(lab2b)]] auto register_wait(int register_cnt = 1) noexcept -> void;

    /**
     * @brief reduce reference count of context
     *
     * @param register_cnt
     */
    [[CORO_TEST_USED(lab2b)]] auto unregister_wait(int register_cnt = 1) noexcept -> void;

    inline auto get_engine() noexcept -> engine& { return m_engine; }

    /**
     * @brief main logic of work thread
     *
     * @param token
     */
    [[CORO_TEST_USED(lab2b)]] auto run(stop_token token) noexcept -> void;

    inline auto is_idle() noexcept -> bool
    {
      return !m_running_task.load(memory_order_acquire)
             && m_engine.empty_io()
             && !m_engine.ready()
             && m_wait_count.load(memory_order_acquire) == 0;
    }

private:
    CORO_ALIGN engine   m_engine;
    unique_ptr<jthread> m_job;
    ctx_id              m_id;
    atomic<int>  m_wait_count{0};  // 引用计数，记录挂起等待的协程数
    atomic<bool> m_running_task{false}; //标记当前是否正在 exec_one_task 执行协程体
    /*
        区分两种模式。无 scheduler 时为 false，context 需要自动停止；有 scheduler 时为
        true，context 必须等 scheduler 发 stop 信号才退出（否则 round-robin 会把任务派回已退出的 context 导致丢任务）。
    */
    bool  m_managed_by_scheduler{false};
    
};

inline context& local_context() noexcept
{
    return *linfo.ctx;
}

inline void submit_to_context(task<void>&& task) noexcept
{
    local_context().submit_task(std::move(task));
}

inline void submit_to_context(task<void>& task) noexcept
{
    local_context().submit_task(task.handle());
}

inline void submit_to_context(std::coroutine_handle<> handle) noexcept
{
    local_context().submit_task(handle);
}

}; // namespace coro
