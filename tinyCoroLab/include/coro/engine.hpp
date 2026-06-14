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

    auto init() noexcept -> void;

    auto deinit() noexcept -> void;

        auto ready() noexcept -> bool;

        [[CORO_DISCARD_HINT]] auto get_free_urs() noexcept -> ursptr;

        auto num_task_schedule() noexcept -> size_t;

        [[CORO_DISCARD_HINT]] auto schedule() noexcept -> coroutine_handle<>;

        auto submit_task(coroutine_handle<> handle) noexcept -> void;

        auto exec_one_task() noexcept -> void;

        auto handle_cqe_entry(urcptr cqe) noexcept -> void;

        auto poll_submit() noexcept -> void;

        auto add_io_submit() noexcept -> void;

        auto empty_io() noexcept -> bool;

        inline auto get_id() noexcept -> uint32_t { return m_id; }

        inline auto get_uring() noexcept -> uring_proxy& { return m_upxy; }

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

inline engine& local_engine() noexcept
{
    return *linfo.egn;
}

}; // namespace coro::detail
