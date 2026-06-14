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

template<typename T>
using AtomicQueue =
    CapacityToConstructor<atomic_queue::AtomicQueueB2<T, std::allocator<T>, true, false, false>, config::kQueCap>;
}; // namespace coro::detail