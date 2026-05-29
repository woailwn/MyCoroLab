#include "coro/engine.hpp"
#include "coro/io/io_info.hpp"
#include "coro/task.hpp"

namespace coro::detail
{
using std::memory_order_relaxed;

auto engine::init() noexcept -> void
{
    m_upxy.init(config::kEntryLength);
    linfo.egn = this;
    //让当前线程通过 local_engine() 能找到这个 engine 
}

auto engine::deinit() noexcept -> void
{
    m_upxy.deinit();
}

auto engine::ready() noexcept -> bool
{
    return m_task_queue.was_size() > 0;
}

auto engine::get_free_urs() noexcept -> ursptr
{
    return m_upxy.get_free_sqe();
}

auto engine::num_task_schedule() noexcept -> size_t
{
    return m_task_queue.was_size();
}

//从任务队列取出一个句柄返回
auto engine::schedule() noexcept -> coroutine_handle<>
{
    return m_task_queue.pop();
}

//把协程句柄放入任务队列
auto engine::submit_task(coroutine_handle<> handle) noexcept -> void
{
    m_task_queue.push(std::move(handle));
    m_upxy.write_eventfd(1);  // 唤醒正在等待的 poll_submit
}

auto engine::exec_one_task() noexcept -> void
{
    auto coro = schedule();
    coro.resume();
    if (coro.done())
    {
        clean(coro);
    }
}

auto engine::handle_cqe_entry(urcptr cqe) noexcept -> void
{
    auto data = reinterpret_cast<io::detail::io_info*>(io_uring_cqe_get_data(cqe));
    data->cb(data, cqe->res);
}

auto engine::poll_submit() noexcept -> void
{
      // 提交待提交的 IO
      auto submit_cnt = m_io_submit_count.load(memory_order_relaxed);
      if (submit_cnt > 0) {
          m_upxy.submit();
          m_io_running_count.fetch_add(submit_cnt, memory_order_relaxed);
          m_io_submit_count.fetch_sub(submit_cnt, memory_order_relaxed);
      }
      
      // 没有运行中的 IO，阻塞等待 eventfd
      if (m_io_running_count == 0) {
          m_upxy.wait_eventfd();  // 阻塞，等新任务或IO完成
          return;
      }   
    
      // 有运行中的 IO，处理完成事件
      if (!m_upxy.peek_uring()) return;
      int nready = m_upxy.peek_batch_cqe(m_urc.data(), m_urc.size());
      for (int i = 0; i < nready; i++) handle_cqe_entry(m_urc[i]);
      m_upxy.cq_advance(nready);
      m_io_running_count.fetch_sub(nready, memory_order_relaxed);
}

auto engine::add_io_submit() noexcept -> void
{
    m_io_submit_count.fetch_add(1, memory_order_relaxed);
}

auto engine::empty_io() noexcept -> bool
{
    return m_io_submit_count == 0 && m_io_running_count == 0;
}
}; // namespace coro::detail
