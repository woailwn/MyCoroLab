#include "coro/scheduler.hpp"

namespace coro
{
auto scheduler::init_impl(size_t ctx_cnt) noexcept -> void
{
    detail::init_meta_info();
    m_ctx_cnt = ctx_cnt;
    m_ctxs    = detail::ctx_container{};
    m_ctxs.reserve(m_ctx_cnt);
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs.emplace_back(std::make_unique<context>());
    }
    m_dispatcher.init(m_ctx_cnt, &m_ctxs);
    for (int i = 0; i < m_ctx_cnt; i++) {
        m_ctxs[i]->m_managed_by_scheduler = true;
    }

#ifdef ENABLE_MEMORY_ALLOC
    coro::allocator::memory::mem_alloc_config config;
    m_mem_alloc.init(config);
    ginfo.mem_alloc = &m_mem_alloc;
#endif
}

auto scheduler::loop_impl() noexcept -> void
{
      for (int i = 0; i < m_ctx_cnt; i++)
          m_ctxs[i]->start();

      while (true) {
            // bool any_running = false;
            // for (int i = 0; i < m_ctx_cnt; i++) {
            //     if (m_ctxs[i]->m_running_task.load(memory_order_acquire))
            //         any_running = true;  // 注意：不要 break，必须全部读完
            // }

            // if (any_running) {
            //     std::this_thread::yield();
            //     continue;
            // }
            bool all_idle = true;
            for (int i = 0; i < m_ctx_cnt; i++) {
                if (!m_ctxs[i]->is_idle()) { all_idle = false; break; }
            }
            if (all_idle) break;
            std::this_thread::yield();
      }

      stop_impl();

      for (int i = 0; i < m_ctx_cnt; i++)
          m_ctxs[i]->join();
}

auto scheduler::stop_impl() noexcept -> void
{
    // This is an example which just notify stop signal to each context,
    // if you don't need this, function just ignore or delete it
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->notify_stop();
    }
}

auto scheduler::submit_task_impl(std::coroutine_handle<> handle) noexcept -> void
{
    size_t ctx_id = m_dispatcher.dispatch();
    m_ctxs[ctx_id]->submit_task(handle);
}
}; // namespace coro
