#pragma once

#include <coroutine>
#include <functional>
#include <queue>

#include "coro/attribute.hpp"
#include "coro/comp/mutex.hpp"
#include "coro/context.hpp"
#include "coro/spinlock.hpp"

namespace coro
{

using cond_type = std::function<bool()>;

class condition_variable;
using cond_var = condition_variable;

class condition_variable final
{
    struct waiter
    {
        std::coroutine_handle<> handle;
        mutex&                  mtx;
        cond_type               cond;   // empty means no condition
    };

    struct awaiter
    {
        condition_variable& m_cv;
        mutex&              m_mtx;
        cond_type           m_cond;

        bool await_ready() noexcept
        {
            if (m_cond && m_cond())
                return true;
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            m_mtx.unlock();
            std::lock_guard<detail::spinlock> lock(m_cv.m_lock);
            m_cv.m_waiters.push({handle, m_mtx, m_cond});
        }

        void await_resume() noexcept {}
    };

public:
    condition_variable() noexcept  = default;
    ~condition_variable() noexcept = default;

    CORO_NO_COPY_MOVE(condition_variable);

    auto wait(mutex& mtx) noexcept -> awaiter
    {
        return awaiter{*this, mtx, {}};
    }

    auto wait(mutex& mtx, cond_type&& cond) noexcept -> awaiter
    {
        return awaiter{*this, mtx, std::move(cond)};
    }

    auto wait(mutex& mtx, cond_type& cond) noexcept -> awaiter
    {
        return awaiter{*this, mtx, cond};
    }

    auto notify_one() noexcept -> void
    {
        std::lock_guard<detail::spinlock> lock(m_lock);
        if (!m_waiters.empty())
        {
            auto w = m_waiters.front();
            m_waiters.pop();
            if (w.cond && !w.cond())
                m_waiters.push(w);
            else
                w.mtx.enqueue_waiter(w.handle);
        }
    }

    auto notify_all() noexcept -> void
    {
        std::lock_guard<detail::spinlock> lock(m_lock);
        size_t cnt = m_waiters.size();
        for (size_t i = 0; i < cnt; i++)
        {
            auto w = m_waiters.front();
            m_waiters.pop();
            if (w.cond && !w.cond())
                m_waiters.push(w);
            else
                w.mtx.enqueue_waiter(w.handle);
        }
    }

private:
    detail::spinlock             m_lock;
    std::queue<waiter>           m_waiters;
};

}; // namespace coro

//调用逻辑
/*
  task<> notify_all_func(test_paras& para, int id)
  {
      auto lock = co_await para.mtx.lock_guard(); // 拿锁
      co_await para.cv.wait(para.mtx, [&]() { return para.id == id; });
      para.vec.push_back(id);
      para.id += 1;
      para.cv.notify_all();
  }

  ---
  初始状态：para.id = 0，提交 id=0,1,2...9 共 10 个协程
  
  id=0 的协程先拿到锁：

  co_await cv.wait(mtx, cond)
    → 构造 awaiter{cv, mtx, cond}
    → await_ready(): cond() = (para.id==0) = true → 直接返回 true
    → 不走 await_suspend，协程不挂起
    → await_resume() 返回，继续执行
    → vec.push_back(0), para.id=1
    → notify_all()
       → 遍历 m_waiters，检查各 waiter 的 cond
       → mtx 释放（lock_guard 析构）

  id=1,2...9 的协程都在等锁或等 cv：

  co_await cv.wait(mtx, cond)
    → await_ready(): cond() = (para.id==1/2..) ≠ 0 = false
    → await_suspend(handle):
        1. mtx.unlock()          ← 释放锁
        2. m_waiters.push({handle, mtx, cond})  ← 进 cv 等待队列
    → 协程挂起

  id=0 调用 notify_all()：

  遍历 m_waiters 中的 9 个 waiter：
    id=1: cond() = (para.id==1) = true  → enqueue_waiter → 重新抢锁
    id=2: cond() = (para.id==2) = false → 放回 cv 等待队列
    id=3~9: 同上，全部放回

  id=1 抢到锁后恢复执行：

  await_resume() 返回
  → vec.push_back(1), para.id=2
  → notify_all()
     → 遍历剩余 8 个 waiter
     → id=2: cond() = true → enqueue_waiter
     → id=3~9: false → 放回
  → lock_guard 析构，释放锁

  依此类推，直到 id=9 执行完。

  ---
  关键路径总结：

  co_await cv.wait(mtx, cond)
           ↓
      await_ready()
      cond 满足 ──→ 直接继续（不挂起，不释放锁）
      cond 不满足
           ↓
      await_suspend()
      unlock(mtx) + 进 cv 等待队列
           ↓
      协程挂起...等 notify
           ↓
      notify 时检查 cond
      满足 → enqueue_waiter（重新抢锁）→ await_resume() → 继续执行
      不满足 → 留在 cv 队列 → 等下次 notify

*/