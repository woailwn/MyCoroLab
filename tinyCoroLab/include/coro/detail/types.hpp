#pragma once

#include <coroutine>
#include <cstdint>

namespace coro::detail
{

// TODO: Add lifo and fifo strategy support
enum class schedule_strategy : uint8_t
{
    fifo, // default
    lifo,
    none
};

enum class dispatch_strategy : uint8_t
{
    round_robin,
    none
};

// TODO: Add awaiter base support
using awaiter_ptr = void*;

using noop_awaiter = std::suspend_always;

enum class memory_allocator : uint8_t
{
    std_allocator,
    none
};

}; // namespace coro::detail