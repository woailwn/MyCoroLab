#pragma once

#include <cassert>
#include <coroutine>
#include <stdexcept>
#include <utility>

#include "coro/attribute.hpp"
#include "coro/detail/container.hpp"

#ifdef ENABLE_MEMORY_ALLOC
    #include "coro/meta_info.hpp"
#endif

namespace coro
{


template<typename return_type = void>
class task;

namespace detail
{
struct promise_base
{
    std::coroutine_handle<> m_parent{nullptr};
    bool m_detached{false}; // 标记是否被detach
    promise_base() noexcept = default;
    ~promise_base()         = default;

    constexpr auto initial_suspend() noexcept { return std::suspend_always{}; }

    auto final_suspend() noexcept
    {
        struct final_awaiter{
            bool await_ready() noexcept {return false;}

            auto await_suspend(std::coroutine_handle<> handle) noexcept -> std::coroutine_handle<>
            {
                auto h = std::coroutine_handle<promise_base>::from_address(handle.address());
                auto parent = h.promise().m_parent;
                if(parent){
                    return parent;
                }
            }

            void await_resume() noexcept {}
        };
        return final_awaiter{};
    }

#ifdef ENABLE_MEMORY_ALLOC
    void* operator new(std::size_t size)
    {
        return ::coro::detail::ginfo.mem_alloc->allocate(size);
    }

    void operator delete(void* ptr, [[CORO_MAYBE_UNUSED]] std::size_t size)
    {
        ::coro::detail::ginfo.mem_alloc->release(ptr);
    }
#endif

#ifdef DEBUG
public:
    int promise_id{0};
#endif // DEBUG
};

template<typename return_type>
struct promise final : public promise_base, public container<return_type>
{
public:
    using task_type        = task<return_type>;
    using coroutine_handle = std::coroutine_handle<promise<return_type>>;

#ifdef DEBUG
    template<typename... Args>
    promise(int id, Args&&... args) noexcept
    {
        promise_id = id;
    }
#endif // DEBUG
    promise() noexcept
    {
    }
    promise(const promise&)             = delete;
    promise(promise&& other)            = delete;
    promise& operator=(const promise&)  = delete;
    promise& operator=(promise&& other) = delete;
    ~promise()                          = default;

    auto get_return_object() noexcept -> task_type;

    auto unhandled_exception() noexcept -> void
    {
        this->set_exception();
    }
};

template<>
struct promise<void> : public promise_base
{
    using task_type        = task<void>;
    using coroutine_handle = std::coroutine_handle<promise<void>>;

#ifdef DEBUG
    template<typename... Args>
    promise(int id, Args&&... args) noexcept
    {
        promise_id = id;
    }
#endif // DEBUG
    promise() noexcept                  = default;
    promise(const promise&)             = delete;
    promise(promise&& other)            = delete;
    promise& operator=(const promise&)  = delete;
    promise& operator=(promise&& other) = delete;
    ~promise()                          = default;

    auto get_return_object() noexcept -> task_type;

    constexpr auto return_void() noexcept -> void
    {
    }

    auto unhandled_exception() noexcept -> void
    {
        m_exception_ptr = std::current_exception();
    }

    auto result() -> void
    {
        if (m_exception_ptr)
        {
            std::rethrow_exception(m_exception_ptr);
        }
    }

private:
    std::exception_ptr m_exception_ptr{nullptr};
};

} // namespace detail

template<typename return_type>
class [[CORO_AWAIT_HINT]] task
{
public:
    using task_type        = task<return_type>;
    using promise_type     = detail::promise<return_type>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    struct awaitable_base
    {
        awaitable_base(coroutine_handle coroutine) noexcept : m_coroutine(coroutine) {}

        auto await_ready() const noexcept -> bool { return !m_coroutine || m_coroutine.done(); }

        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> std::coroutine_handle<>
        {
            //awaiting_coroutine是父协程的句柄，存在子协程的promise
            m_coroutine.promise().m_parent = awaiting_coroutine;
            return m_coroutine;//直接返回子协程
        }

        std::coroutine_handle<promise_type> m_coroutine{nullptr};
    };

    task() noexcept : m_coroutine(nullptr) {}

    explicit task(coroutine_handle handle) : m_coroutine(handle) {}
    task(const task&) = delete;
    task(task&& other) noexcept : m_coroutine(std::exchange(other.m_coroutine, nullptr)) {}

    ~task()
    {
        if (m_coroutine != nullptr)
        {
            m_coroutine.destroy();
        }
    }

    auto operator=(const task&) -> task& = delete;

    auto operator=(task&& other) noexcept -> task&
    {
        if (std::addressof(other) != this)
        {
            if (m_coroutine != nullptr)
            {
                m_coroutine.destroy();
            }

            m_coroutine = std::exchange(other.m_coroutine, nullptr);
        }

        return *this;
    }

    /**
     * @return True if the task is in its final suspend or if the task has been destroyed.
     */
    auto is_ready() const noexcept -> bool { return m_coroutine == nullptr || m_coroutine.done(); }

    auto resume() -> bool
    {
        if (!m_coroutine.done())
        {
            m_coroutine.resume();
        }
        return !m_coroutine.done();
    }

    auto destroy() -> bool
    {
        if (m_coroutine != nullptr)
        {
            m_coroutine.destroy();
            m_coroutine = nullptr;
            return true;
        }

        return false;
    }

    //语义为：转移释放业务
    /*
        如果不 detach，task 析构时把协程帧销毁了，执行引擎拿着一个野指针
        resume 时就会coredump
    */
    auto detach() -> void
    {
        m_coroutine.promise().m_detached = true;
        m_coroutine=nullptr;
    }

    auto operator co_await() const& noexcept
    {
        struct awaitable : public awaitable_base
        {
            auto await_resume() -> decltype(auto) { return this->m_coroutine.promise().result(); }
        };

        return awaitable{m_coroutine};
    }

    auto operator co_await() const&& noexcept
    {
        struct awaitable : public awaitable_base
        {
            auto await_resume() -> decltype(auto) { return std::move(this->m_coroutine.promise()).result(); }
        };

        return awaitable{m_coroutine};
    }

    auto promise() & -> promise_type& { return m_coroutine.promise(); }
    auto promise() const& -> const promise_type& { return m_coroutine.promise(); }
    auto promise() && -> promise_type&& { return std::move(m_coroutine.promise()); }

    auto handle() & -> coroutine_handle { return m_coroutine; }
    auto handle() && -> coroutine_handle { return std::exchange(m_coroutine, nullptr); }

private:
    coroutine_handle m_coroutine{nullptr};
};

using coroutine_handle = std::coroutine_handle<detail::promise_base>;

/**
 * 
 * @brief 语义为：执行引擎用完后释放
 *
 * @param handle
 */
inline auto clean(std::coroutine_handle<> handle) noexcept -> void
{
    auto h = std::coroutine_handle<detail::promise_base>::from_address(handle.address());
    if(h.promise().m_detached){
        handle.destroy();
    }
}

namespace detail
{
template<typename return_type>
inline auto promise<return_type>::get_return_object() noexcept -> task<return_type>
{
    return task<return_type>{coroutine_handle::from_promise(*this)};
}

inline auto promise<void>::get_return_object() noexcept -> task<>
{
    return task<>{coroutine_handle::from_promise(*this)};
}

#ifdef DEBUG
template<typename T = void>
inline auto get_promise(std::coroutine_handle<> handle) -> promise<T>&
{
    auto  specific_handle = std::coroutine_handle<detail::promise<T>>::from_address(handle.address());
    auto& promise         = specific_handle.promise();
    return promise;
}
#endif // DEBUG

} // namespace detail

} // namespace coro
