#pragma once

#include <cstddef>
#include <memory>

#include "config.h"
#include "coro/attribute.hpp"
#include "coro/detail/types.hpp"

namespace coro::allocator::memory
{
template<coro::detail::memory_allocator alloc_strategy>
class memory_allocator
{
public:
    struct config
    {
    };

public:
    ~memory_allocator() = default;

    auto init(config config) -> void {}

    auto allocate(size_t size) -> void* { return nullptr; }

    auto release(void* ptr) -> void {}
};

template<>
class memory_allocator<coro::config::kMemoryAllocator>
{
public:
    struct config
    {
    };

public:
    ~memory_allocator() = default;

    auto init(config config) -> void {}

    auto allocate(size_t size) -> void* CORO_INLINE { return malloc(size); }

    auto release(void* ptr) -> void CORO_INLINE { free(ptr); }
};

// This config type is used to provide for outsides
using mem_alloc_config = memory_allocator<coro::config::kMemoryAllocator>::config;

}; // namespace coro::allocator::memory
