#include "coro/context.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
context::context() noexcept
{
    m_id = ginfo.context_id.fetch_add(1, std::memory_order_relaxed);
}

auto context::init() noexcept -> void
{
    m_engine.init();
    linfo.ctx = this;
}

auto context::deinit() noexcept -> void
{
    linfo.ctx = nullptr;
    m_engine.deinit();
}

auto context::start() noexcept -> void
{
    m_job = make_unique<jthread>(
        [this](stop_token token)
        {
            this->init();
            this->run(token);
            this->deinit();
        });
}

auto context::notify_stop() noexcept -> void
{
    m_job->request_stop();          // 发停止信号
    m_engine.get_uring().write_eventfd(1);  // 唤醒可能阻塞在 wait_eventfd 的工作线程
}

auto context::submit_task(std::coroutine_handle<> handle) noexcept -> void
{
    register_wait(1);
    m_engine.submit_task(handle);
}

auto context::register_wait(int register_cnt) noexcept -> void
{
    m_wait_count.fetch_add(register_cnt, memory_order_relaxed);
}

auto context::unregister_wait(int register_cnt) noexcept -> void
{
    m_wait_count.fetch_sub(register_cnt, memory_order_relaxed);
    m_engine.get_uring().write_eventfd(1);  // 唤醒工作线程重新检查
}

auto context::run(stop_token token) noexcept -> void
{
    //满足的条件（没有停止，engine没有IO请求处理或未处理，任务队列不为空，挂起的任务不为0）
      while (!token.stop_requested() || !m_engine.empty_io() || m_engine.ready() || m_wait_count > 0) {
          while (m_engine.ready()) {
            m_running_task.store(true, memory_order_release); 
            m_engine.exec_one_task();
            unregister_wait(1);
            m_running_task.store(false, memory_order_release);
          }
          if (!m_managed_by_scheduler && !token.stop_requested()
              && m_engine.empty_io() && !m_engine.ready() && m_wait_count == 0) {
              notify_stop();
          }
          m_engine.poll_submit();
      }
}

}; // namespace coro