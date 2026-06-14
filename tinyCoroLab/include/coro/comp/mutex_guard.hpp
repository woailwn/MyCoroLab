#pragma once

#include "coro/concepts/lock.hpp"

namespace coro::detail
{

// 修复: Why locktype report error?
// template<concepts::lockype T>
template<typename T>
class lock_guard
{
public:
    explicit lock_guard(T& mtx) noexcept : m_mtx(mtx) {}
    ~lock_guard() noexcept { m_mtx.unlock(); }

    lock_guard(lock_guard&& other) noexcept : m_mtx(other.m_mtx) {}

    lock_guard(const lock_guard&)            = delete;
    lock_guard& operator=(const lock_guard&) = delete;
    lock_guard& operator=(lock_guard&&)      = delete;

private:
    T& m_mtx;
};

}; // namespace coro::detail