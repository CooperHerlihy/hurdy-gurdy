#pragma once

#include <atomic>
#include <thread>
#include <type_traits>
#include <utility>

#include "hg/inttypes.hpp"

namespace hg {

/**
 * A spinlock mutex for basic thread synchronization
 */
struct SpinLock {
    /**
     * Whether the lock is currently acquired
     */
    std::atomic_bool acquired{false};

    /**
     * Wait until the mutex is acquired
     */
    void acquire();

    /**
     * Try to acquire the mutex
     *
     * Returns
     * - true if acquisition succeeded
     * - false if the mutex was already in use
     */
    bool tryAcquire();

    /**
     * Release the mutex lock
     */
    void release();
};

/**
 * A scoped lock guard for SpinLock
 *
 * Acquires the lock on construction, releases on destruction.
 */
struct SpinLockScope {
    /**
     * The spinlock to manage
     */
    SpinLock* lock = nullptr;

    /**
     * Construct empty
     */
    SpinLockScope() noexcept = default;

    /**
     * Acquire the lock
     *
     * Parameters
     * - lockVal The spinlock to acquire
     */
    SpinLockScope(SpinLock& lockVal)
        : lock{&lockVal}
    {
        if (lock != nullptr)
            lock->acquire();
    }

    /**
     * Release the lock
     */
    ~SpinLockScope() noexcept
    {
        if (lock != nullptr)
            lock->release();
    }

    /**
     * Move construct
     */
    SpinLockScope(SpinLockScope&& other) noexcept
        : lock{std::exchange(other.lock, nullptr)}
    {}

    /**
     * Move assign
     */
    SpinLockScope& operator=(SpinLockScope&& other) noexcept
    {
        if (this != &other)
        {
            this->~SpinLockScope();
            new (this) SpinLockScope{std::move(other)};
        }
        return *this;
    }

    SpinLockScope(const SpinLockScope&) = delete;
    SpinLockScope& operator=(const SpinLockScope&) = delete;
};

/**
 * A spinlock fence for basic thread synchronization
 */
struct Fence {
    /**
     * How many events are being waited on
     */
    std::atomic<u32> counter{0};

    /**
     * Add more events for the fence to wait on
     *
     * Parameters
     * - count The number of added events
     */
    void add(u32 count = 1);

    /**
     * Signal that events have completed
     *
     * Parameters
     * - count The number of signaled events
     */
    void signal(u32 count = 1);

    /**
     * Returns whether all work has been completed
     */
    bool isComplete();

    /**
     * Spin waits for all work submissions to be completed
     *
     * Parameters
     * - timeout The time in seconds to wait before timing out
     *
     * Returns
     * - true if the fence was completed, false if the timeout was triggered
     */
    bool wait(f64 timeout);

    /**
     * Spin waits for all work submissions to be completed
     */
    void waitIndefinite();
};

/**
 * Wait on a fence, and help complete work in the meantime
 *
 * Parameters
 * - fence The fence to wait on
 * - timeout The max time in seconds to spend working
 *
 * Returns
 * - true if the fence was completed
 * - false if the timeout was reached
 */
bool helpThreads(Fence* fence, f64 timeout);

/**
 * Pushes work to the thread pool queue to be executed
 *
 * Parameters
 * - fence The fences to signal upon completion
 * - data The data passed to the function
 * - work The function to be executed
 */
void callPar(Fence* fence, void* data, void (*fn)(void* data));

/**
 * Iterates in parallel over a function n times using the thread pool
 *
 * Note, uses a fence internally to wait for all work to complete
 *
 * Parameters
 * - begin The first index to iterate from
 * - end The end index to iterate to
 * - data The data pointer passed to fn
 * - fn The function to use to iterate, takes the index
 */
void forPar(u64 begin, u64 end, void* data, void (*fn)(void* data, u64 idx));

/**
 * Iterates in parallel over a function n times using the thread pool
 *
 * Note, uses a fence internally to wait for all work to complete
 *
 * Parameters
 * - begin The first index to iterate from
 * - end The end index to iterate to
 * - fn The function to use to iterate, takes the index
 */
template<typename F> requires std::is_invocable_r_v<void, F, u64>
void forPar(u64 begin, u64 end, F fn)
{
    forPar(begin, end, &fn, [](void* pfn, u64 idx)
    {
        (*static_cast<F*>(pfn))(idx);
    });
}

} // namespace hg

