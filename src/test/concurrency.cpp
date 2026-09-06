#include "tests.hpp"
#include "hg/concurrency.hpp"

using namespace hg;

TEST(testSpinLockAcquireRelease)
{
    SpinLock lock{};
    lock.acquire();
    lock.release();

    bool ok = lock.tryAcquire();
    ASSERT(ok);
    lock.release();

    lock.acquire();
    ok = lock.tryAcquire();
    ASSERT(!ok);
    lock.release();
}

TEST(testSpinLockRepeated)
{
    SpinLock lock{};
    for (u32 i = 0; i < 100; ++i)
    {
        lock.acquire();
        lock.release();
    }
}

TEST(testSpinLockTryAcquireCycle)
{
    SpinLock lock{};
    for (u32 i = 0; i < 100; ++i)
    {
        bool ok = lock.tryAcquire();
        ASSERT(ok);
        lock.release();
    }
}

TEST(testFenceAddSignalWait)
{
    Fence fence{};
    ASSERT(fence.isComplete());

    fence.add();
    ASSERT(!fence.isComplete());
    fence.signal();
    ASSERT(fence.isComplete());

    fence.add(5);
    ASSERT(!fence.isComplete());
    fence.signal(3);
    ASSERT(!fence.isComplete());
    fence.signal(2);
    ASSERT(fence.isComplete());

    bool ok = fence.wait(1.0);
    ASSERT(ok);

    fence.add();
    ok = fence.wait(0.0001);
    ASSERT(!ok);
    fence.signal();
}

TEST(testFenceWaitIndefinite)
{
    Fence fence{};
    fence.add();
    std::thread t{[&fence] { fence.signal(); }};
    fence.waitIndefinite();
    ASSERT(fence.isComplete());
    t.join();
}

TEST(testFenceAddSignalZero)
{
    Fence fence{};
    fence.add(0);
    ASSERT(fence.isComplete());
    fence.add();
    fence.signal(0);
    ASSERT(!fence.isComplete());
    fence.signal();
    ASSERT(fence.isComplete());
}

TEST(testSpinLockScopeRAII)
{
    SpinLock lock{};
    {
        SpinLockScope scope{lock};
    }
    lock.acquire();
    lock.release();
}

TEST(testSpinLockScopeNull)
{
    SpinLockScope scope{};
    ASSERT(scope.lock == nullptr);
}

TEST(testSpinLockScopeMoveConstruct)
{
    SpinLock lock{};
    SpinLockScope inner{lock};
    SpinLockScope moved{std::move(inner)};
    ASSERT(inner.lock == nullptr);
    ASSERT(moved.lock == &lock);
}

TEST(testSpinLockScopeMoveAssign)
{
    SpinLock lock{};
    SpinLockScope a{lock};
    SpinLockScope b{};
    b = std::move(a);
    ASSERT(a.lock == nullptr);
    ASSERT(b.lock == &lock);
}

TEST(testCallParSingleAndMultiple)
{
    Fence fence{};
    bool executed = false;
    callPar(&fence, &executed, [](void* data)
    {
        *static_cast<bool*>(data) = true;
    });
    bool ok = fence.wait(2.0);
    ASSERT(ok);
    ASSERT(executed);

    bool a = false, b = false, c = false;
    Fence fence2{};
    callPar(&fence2, &a, [](void* p) { *static_cast<bool*>(p) = true; });
    callPar(&fence2, &b, [](void* p) { *static_cast<bool*>(p) = true; });
    callPar(&fence2, &c, [](void* p) { *static_cast<bool*>(p) = true; });
    ok = fence2.wait(2.0);
    ASSERT(ok);
    ASSERT(a && b && c);
}

TEST(testCallParNullFence)
{
    callPar(nullptr, nullptr, [](void*) {});
    Fence fence{};
    bool executed = false;
    callPar(&fence, &executed, [](void* data)
    {
        *static_cast<bool*>(data) = true;
    });
    fence.wait(2.0);
    ASSERT(executed);
}

TEST(testCallParManyItems)
{
    Fence fence{};
    static constexpr u32 count = 100;
    bool vals[count]{};
    for (u32 i = 0; i < count; ++i)
    {
        callPar(&fence, &vals[i], [](void* p)
        {
            *static_cast<bool*>(p) = true;
        });
    }
    bool ok = fence.wait(2.0);
    ASSERT(ok);
    for (u32 i = 0; i < count; ++i)
        ASSERT(vals[i]);
}

TEST(testCallParModifyPointer)
{
    Fence fence{};
    u32 result = 0;
    callPar(&fence, &result, [](void* p)
    {
        *static_cast<u32*>(p) = 42;
    });
    fence.wait(2.0);
    ASSERT(result == 42);
}

TEST(testHelpThreadsSingle)
{
    Fence fence{};
    bool executed = false;
    callPar(&fence, &executed, [](void* data)
    {
        *static_cast<bool*>(data) = true;
    });
    bool ok = helpThreads(&fence, 2.0);
    ASSERT(ok);
    ASSERT(executed);
}

TEST(testHelpThreadsMany)
{
    Fence fence{};
    static constexpr u32 count = 100;
    bool vals[count]{};
    for (u32 i = 0; i < count; ++i)
    {
        callPar(&fence, &vals[i], [](void* p)
        {
            *static_cast<bool*>(p) = true;
        });
    }
    bool ok = helpThreads(&fence, 2.0);
    ASSERT(ok);
    for (u32 i = 0; i < count; ++i)
        ASSERT(vals[i]);
}

TEST(testForParCallback)
{
    static constexpr u64 count = 100;
    bool vals[count]{};
    forPar(u64{0}, u64{count}, vals, [](void* data, u64 idx)
    {
        static_cast<bool*>(data)[idx] = true;
    });
    for (u64 i = 0; i < count; ++i)
        ASSERT(vals[i]);

    u32 ints[count]{};
    forPar(u64{0}, u64{count}, ints, [](void* data, u64 idx)
    {
        static_cast<u32*>(data)[idx] = static_cast<u32>(idx + 1);
    });
    for (u64 i = 0; i < count; ++i)
        ASSERT(ints[i] == i + 1);
}

TEST(testForParLargeRange)
{
    static constexpr u64 count = 1000;
    std::atomic<u32> sum{0};
    forPar(u64{0}, u64{count}, [&](u64)
    {
        sum.fetch_add(1);
    });
    ASSERT(sum.load() == count);
}

TEST(testForParReferenceCapture)
{
    static constexpr u64 count = 500;
    u32 vals[count]{};
    forPar(u64{0}, u64{count}, [&](u64 idx)
    {
        vals[idx] = static_cast<u32>(idx * 2);
    });
    for (u64 i = 0; i < count; ++i)
        ASSERT(vals[i] == i * 2);
}

TEST(testConcurrencyStress)
{
    for (u32 concurrencyIter = 0; concurrencyIter < 3; ++concurrencyIter)
    {

    // Concurrent producers: 2 threads
    {
        Fence fence{};
        static constexpr u32 count = 50;
        bool vals[count]{};

        std::thread t1{[&]
        {
            for (u32 i = 0; i < count / 2; ++i)
                callPar(&fence, &vals[i], [](void* p)
                {
                    *static_cast<bool*>(p) = true;
                });
        }};
        std::thread t2{[&]
        {
            for (u32 i = count / 2; i < count; ++i)
                callPar(&fence, &vals[i], [](void* p)
                {
                    *static_cast<bool*>(p) = true;
                });
        }};
        t1.join();
        t2.join();
        bool ok = helpThreads(&fence, 2.0);
        ASSERT(ok);
        for (u32 i = 0; i < count; ++i)
            ASSERT(vals[i]);
    }

    // Four threads producing work
    {
        Fence fence{};
        static constexpr u32 count = 200;
        bool vals[count]{};
        static constexpr u32 threadCount = 4;

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
        {
            threads[t] = std::thread{[&, t]
            {
                u32 begin = (count / threadCount) * t;
                u32 end = (t == threadCount - 1) ? count : begin + count / threadCount;
                for (u32 i = begin; i < end; ++i)
                    callPar(&fence, &vals[i], [](void* p)
                    {
                        *static_cast<bool*>(p) = true;
                    });
            }};
        }
        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();
        bool ok = helpThreads(&fence, 2.0);
        ASSERT(ok);
        for (u32 i = 0; i < count; ++i)
            ASSERT(vals[i]);
    }

    // Concurrent fence add/signal from 8 threads
    {
        Fence fence{};
        static constexpr u32 threadCount = 8;
        static constexpr u32 signalsPerThread = 100;
        fence.add(threadCount * signalsPerThread);

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
        {
            threads[t] = std::thread{[&fence]
            {
                for (u32 i = 0; i < signalsPerThread; ++i)
                    fence.signal();
            }};
        }
        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();
        ASSERT(fence.isComplete());
    }

    // SpinLock multi-threaded mutual exclusion
    {
        SpinLock lock{};
        u32 shared = 0;
        static constexpr u32 threadCount = 4;
        static constexpr u32 incrementsPerThread = 5000;

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
            threads[t] = std::thread{[&]
            {
                for (u32 i = 0; i < incrementsPerThread; ++i)
                {
                    lock.acquire();
                    ++shared;
                    lock.release();
                }
            }};
        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();
        ASSERT(shared == threadCount * incrementsPerThread);
    }

    // SpinLockScope multi-threaded with RAII guard
    {
        SpinLock lock{};
        u32 shared = 0;
        static constexpr u32 threadCount = 4;
        static constexpr u32 incrementsPerThread = 5000;

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
            threads[t] = std::thread{[&]
            {
                for (u32 i = 0; i < incrementsPerThread; ++i)
                {
                    SpinLockScope scope{lock};
                    ++shared;
                }
            }};
        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();
        ASSERT(shared == threadCount * incrementsPerThread);
    }

    } // concurrencyIter
}
