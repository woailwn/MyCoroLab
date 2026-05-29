#pragma once

#include <atomic>

#if defined(_MSC_VER)
    #include <immintrin.h>
#endif

namespace coro::detail
{
using std::atomic;
using std::memory_order_acquire;
using std::memory_order_relaxed;
using std::memory_order_release;

struct spinlock
{
    atomic<bool> lock_ = {0};

    void lock() noexcept
    {
        for (;;)
        {
            // Optimistically assume the lock is free on the first try
            if (!lock_.exchange(true, memory_order_acquire))
            {
                return;
            }
            // Wait for lock to be released without generating cache misses
            while (lock_.load(memory_order_relaxed))
            {
                // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
                // hyper-threads

#if defined(__GNUC__) || defined(__clang__)
                __builtin_ia32_pause();
#elif defined(_MSC_VER)
                _mm_pause();
#else
                __builtin_ia32_pause();
#endif
            }
        }
    }

    bool try_lock() noexcept
    {
        // First do a relaxed load to check if lock is free in order to prevent
        // unnecessary cache misses if someone does while(!try_lock())
        return !lock_.load(memory_order_relaxed) && !lock_.exchange(true, memory_order_acquire);
    }

    void unlock() noexcept { lock_.store(false, memory_order_release); }
};

}; // namespace coro