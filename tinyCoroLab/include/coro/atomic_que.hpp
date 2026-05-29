#include <cstddef>

#include "config.h"

#include "atomic_queue/atomic_queue.h"

namespace coro::detail
{
// lib AtomicQueue
template<class Queue, size_t Capacity>
struct CapacityToConstructor : Queue
{
    CapacityToConstructor() : Queue(Capacity) {}
};

/**
 * @brief mpmc atomic_queue
 *
 * @tparam T storage_type
 *
 * @note true, false, false: MAXIMIZE_THROUGHPUT = true, TOTAL_ORDER = false, bool SPSC = false
 */
template<typename T>
using AtomicQueue =
    CapacityToConstructor<atomic_queue::AtomicQueueB2<T, std::allocator<T>, true, false, false>, config::kQueCap>;
}; // namespace coro::detail