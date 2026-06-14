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

    auto init() noexcept -> void;

    auto deinit() noexcept -> void;

        auto start() noexcept -> void;

        auto notify_stop() noexcept -> void;

        inline auto join() noexcept -> void { m_job->join(); }

    inline auto submit_task(task<void>&& task) noexcept -> void
    {
        auto handle = task.handle();
        task.detach();
        this->submit_task(handle);
    }

    inline auto submit_task(task<void>& task) noexcept -> void { submit_task(task.handle()); }

        auto submit_task(std::coroutine_handle<> handle) noexcept -> void;

        inline auto get_ctx_id() noexcept -> ctx_id { return m_id; }

        auto register_wait(int register_cnt = 1) noexcept -> void;

        auto unregister_wait(int register_cnt = 1) noexcept -> void;

    inline auto get_engine() noexcept -> engine& { return m_engine; }

        auto run(stop_token token) noexcept -> void;

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
