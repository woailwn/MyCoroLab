/**
 * @file engine.hpp
 * @author JiahuiWang
 * @brief lab2a
 * @version 1.1
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <array>
#include <atomic>
#include <coroutine>
#include <functional>
#include <queue>

#include "config.h"
#include "coro/atomic_que.hpp"
#include "coro/attribute.hpp"
#include "coro/meta_info.hpp"
#include "coro/uring_proxy.hpp"

namespace coro
{
class context;
};

/**
 * @brief Welcome to tinycoro lab2a, in this part you will build the heart of tinycoro¡ª¡ªengine by
 * modifing engine.hpp and engine.cpp, please ensure you have read the document of lab2a.
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

namespace coro::detail
{
using std::array;
using std::atomic;
using std::coroutine_handle;
using std::queue;
using uring::urcptr;
using uring::uring_proxy;
using uring::ursptr;

template<typename T>
// multi producer and multi consumer queue
using mpmc_queue = AtomicQueue<T>;

class engine
{
    friend class ::coro::context;

public:
    engine() noexcept { m_id = ginfo.engine_id.fetch_add(1, std::memory_order_relaxed); }

    ~engine() noexcept = default;

    // forbidden to copy and move
    engine(const engine&)                    = delete;
    engine(engine&&)                         = delete;
    auto operator=(const engine&) -> engine& = delete;
    auto operator=(engine&&) -> engine&      = delete;

    [[CORO_TEST_USED(lab2a)]] auto init() noexcept -> void;

    [[CORO_TEST_USED(lab2a)]] auto deinit() noexcept -> void;

    /**
     * @brief return if engine has task to run
     *
     * @return true
     * @return false
     */
    [[CORO_TEST_USED(lab2a)]] auto ready() noexcept -> bool;

    /**
     * @brief get the free uring sqe entry
     *
     * @return ursptr
     */
    [[CORO_DISCARD_HINT]] auto get_free_urs() noexcept -> ursptr;

    /**
     * @brief return number of task to run in engine
     *
     * @return size_t
     */
    [[CORO_TEST_USED(lab2a)]] auto num_task_schedule() noexcept -> size_t;

    /**
     * @brief fetch one task handle and engine should erase it from its queue
     *
     * @return coroutine_handle<>
     */
    [[CORO_TEST_USED(lab2a)]] [[CORO_DISCARD_HINT]] auto schedule() noexcept -> coroutine_handle<>;

    /**
     * @brief submit one task handle to engine
     *
     * @param handle
     */
    [[CORO_TEST_USED(lab2a)]] auto submit_task(coroutine_handle<> handle) noexcept -> void;

    /**
     * @brief this will call schedule() to fetch one task handle and run it
     *
     * @note this function will call clean() when handle is done
     */
    auto exec_one_task() noexcept -> void;

    /**
     * @brief handle one cqe entry
     *
     * @param cqe io_uring cqe entry
     */
    auto handle_cqe_entry(urcptr cqe) noexcept -> void;

    /**
     * @brief submit uring sqe and wait uring finish, then handle
     * cqe entry by call handle_cqe_entry
     *
     */
    [[CORO_TEST_USED(lab2a)]] auto poll_submit() noexcept -> void;

    /**
     * @brief tell engine there has one io to be submitted, engine will record this
     *
     */
    [[CORO_TEST_USED(lab2a)]] auto add_io_submit() noexcept -> void;

    /**
     * @brief return if there has any io to be processed,
     * io include two types: submit_io, running_io
     *
     * @note submit_io: io to be submitted
     * @note running_io: io is submitted but it has't run finished
     *
     * @return true
     * @return false
     */
    [[CORO_TEST_USED(lab2a)]] auto empty_io() noexcept -> bool;

    /**
     * @brief return engine unique id
     *
     * @return uint32_t
     */
    inline auto get_id() noexcept -> uint32_t { return m_id; }

    /**
     * @brief Get the uring object owned by engine, so the outside can use
     * some function of uring
     *
     * @return uring_proxy&
     */
    inline auto get_uring() noexcept -> uring_proxy& { return m_upxy; }

    // TODO[lab2a]: Add more function if you need

private:
    uint32_t    m_id;
    uring_proxy m_upxy;

    // store task handle
    mpmc_queue<coroutine_handle<>> m_task_queue; // You can replace it with another data structure

    // used to fetch cqe entry
    array<urcptr, config::kQueCap> m_urc;

    atomic<size_t> m_io_submit_count{0};   // ???? IO ?
    atomic<size_t> m_io_running_count{0};  // ???????? IO ?
};

/**
 * @brief return local thread engine
 *
 * @return engine&
 */
inline engine& local_engine() noexcept
{
    return *linfo.egn;
}

}; // namespace coro::detail
