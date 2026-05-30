/**
 * @file event.hpp
 * @author JiahuiWang
 * @brief lab4a
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

#include "coro/attribute.hpp"
#include "coro/concepts/awaitable.hpp"
#include "coro/context.hpp"
#include "coro/detail/container.hpp"
#include "coro/detail/types.hpp"
#include "coro/spinlock.hpp"
#include "coro/scheduler.hpp"
namespace coro
{
/**
 * @brief Welcome to tinycoro lab4a, in this part you will build the basic coroutine
 * synchronization component - event by modifing event.hpp and event.cpp. Please ensure
 * you have read the document of lab4a.
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

namespace detail
{
// TODO[lab4a]: Add code that you don't want to use externally in namespace detail
}; // namespace detail


//有值版
template<typename return_type = void>
class event
{   
    struct awaiter
    {
        event& m_event;
        bool await_ready(){
            //判断event是否已经被set
            return m_event.m_is_set.load(std::memory_order_acquire);
        }

        void await_suspend(std::coroutine_handle<> handle){
            std::lock_guard<detail::spinlock> lock(m_event.m_lock);
            if(m_event.m_is_set.load(std::memory_order_acquire)){
                submit_to_scheduler(handle);
                return;
            }
            m_event.m_waiters.push_back(handle);
        }
        auto await_resume() -> return_type { return m_event.m_value; }
    };

public:
    auto wait() noexcept -> awaiter { return awaiter(*this); } 

    template<typename value_type>
    auto set(value_type&& value) noexcept -> void
    {
        std::lock_guard<detail::spinlock> lock(m_lock);
        m_value = std::forward<value_type>(value);
        m_is_set.store(true,std::memory_order_release);
        //唤醒所有等待的协程
        for(auto handle:m_waiters){
            submit_to_scheduler(handle);
        }
        m_waiters.clear();
    }

private:
    atomic<bool> m_is_set{false};         //是否已经被触发
    return_type m_value;                  //存储set传入的值
    detail::spinlock m_lock;              //保护等待列表
    std::vector<std::coroutine_handle<>> m_waiters; //等待者列表
};

// void 特化版：仅传递信号，co_await wait() 返回 void
template<>
class event<>
{
    struct awaiter
    {
        event& m_event;

        bool await_ready() noexcept
        {
            return m_event.m_is_set.load(std::memory_order_acquire);
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            std::lock_guard<detail::spinlock> lock(m_event.m_lock);
            if (m_event.m_is_set.load(std::memory_order_acquire))
            {
                submit_to_scheduler(handle);
                return;
            }
            m_event.m_waiters.push_back(handle);
        }

        void await_resume() noexcept {}
    };

public:
    auto wait() noexcept -> awaiter { return awaiter{*this}; }

    auto set() noexcept -> void
    {
        std::lock_guard<detail::spinlock> lock(m_lock);
        m_is_set.store(true, std::memory_order_release);
        for (auto handle : m_waiters)
        {
            submit_to_scheduler(handle);
        }
        m_waiters.clear();
    }

private:
    std::atomic<bool>                    m_is_set{false};
    detail::spinlock                     m_lock;
    std::vector<std::coroutine_handle<>> m_waiters;
};

// RAII 包装：析构时自动调用 set()，确保等待者一定被唤醒
class event_guard
{
    using guard_type = event<>;

public:
    event_guard(guard_type& ev) noexcept : m_ev(ev) {}
    ~event_guard() noexcept { m_ev.set(); }

private:
    guard_type& m_ev;
};

}; // namespace coro
