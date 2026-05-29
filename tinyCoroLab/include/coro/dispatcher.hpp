#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "coro/context.hpp"
#include "coro/detail/types.hpp"

namespace coro::detail
{
using ctx_container = std::vector<std::unique_ptr<context>>;

// template<typename dispatcher>
// concept dispatcher_type = requires(dispatcher t) {
//     { t.dispatch_impl() } -> std::same_as<size_t>;
// };

// template<dispatcher_type dispatcher>
// class dispatcher_base
// {
// public:
//     dispatcher_base(const size_t ctx_cnt, ctx_container& ctxs) noexcept : m_ctx_cnt(ctx_cnt), m_ctxs(ctxs) {}

//     size_t dispatch() noexcept { return static_cast<dispatcher*>(this)->dispatch_impl(); }

// protected:
//     const size_t   m_ctx_cnt;
//     ctx_container& m_ctxs;
// };

template<dispatch_strategy dst>
class dispatcher
{
public:
    // Each strategy must impl init and dispatch.

    /**
     * @brief
     *
     * @param ctx_cnt
     * @param ctxs
     */
    void init([[CORO_MAYBE_UNUSED]] const size_t ctx_cnt, [[CORO_MAYBE_UNUSED]] ctx_container* ctxs) noexcept {}

    /**
     * @brief Choose one context to schedule by specific strategy.
     * The impl needs to ensure thread safety.
     *
     * @return size_t
     */
    auto dispatch() noexcept -> size_t { return 0; }
};

/**
 * @brief round robin is an easy load balance algorithm
 *
 * @tparam
 */
template<>
class dispatcher<dispatch_strategy::round_robin>
{
public:
    void init(size_t ctx_cnt, [[CORO_MAYBE_UNUSED]] ctx_container* ctxs) noexcept
    {
        m_ctx_cnt = ctx_cnt;
        m_cur     = 0;
    }

    auto dispatch() noexcept -> size_t { return m_cur.fetch_add(1, std::memory_order_acq_rel) % m_ctx_cnt; }

private:
    size_t              m_ctx_cnt;
    std::atomic<size_t> m_cur{0};
};

}; // namespace coro::detail
