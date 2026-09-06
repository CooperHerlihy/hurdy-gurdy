#include "tests.hpp"
#include "hg/ecs.hpp"

using namespace hg;

TEST(testForEachSingleEntityAndRef)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<u32>(e2) = 20;

    u64 sum = 0;
    ecs.forEach<u32>([&](Entity e, u32& v)
    {
        sum += v;
        (void)e;
    });
    ASSERT(sum == 30);
}

TEST(testForEachSingleEntityOnly)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<u32>(e2) = 20;

    u64 sum = 0;
    ecs.forEach<u32>([&](Entity e)
    {
        sum += ecs.get<u32>(e);
    });
    ASSERT(sum == 30);
}

TEST(testForEachSingleRefOnly)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<u32>(e2) = 20;

    u64 sum = 0;
    ecs.forEach<u32>([&](u32& v)
    {
        sum += v;
    });
    ASSERT(sum == 30);
}

TEST(testForEachParSingleEntityAndRef)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<u32>(e2) = 20;

    SpinLock lk;
    u64 sum = 0;
    ecs.forEachPar<u32>([&](Entity e, u32& v)
    {
        SpinLockScope scope{lk};
        sum += v;
        (void)e;
    });
    ASSERT(sum == 30);
}

TEST(testForEachParSingleEntityOnly)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<u32>(e2) = 20;

    SpinLock lk;
    u64 sum = 0;
    ecs.forEachPar<u32>([&](Entity e)
    {
        SpinLockScope scope{lk};
        sum += ecs.get<u32>(e);
    });
    ASSERT(sum == 30);
}

TEST(testForEachParSingleRefOnly)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<u32>(e2) = 20;

    SpinLock lk;
    u64 sum = 0;
    ecs.forEachPar<u32>([&](u32& v)
    {
        SpinLockScope scope{lk};
        sum += v;
    });
    ASSERT(sum == 30);
}

TEST(testForEachMultiEntityAndRef)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    Entity e3 = ecs.spawn();
    ecs.add<u32>(e1) = 1;
    ecs.add<f32>(e1) = 10.0f;
    ecs.add<u32>(e2) = 2;
    ecs.add<f32>(e2) = 20.0f;
    ecs.add<u32>(e3) = 3;

    u64 sum = 0;
    ecs.forEach<u32, f32>([&](Entity e, u32& a, f32& b)
    {
        sum += a + (u64)b;
        (void)e;
    });
    ASSERT(sum == 33);
}

TEST(testForEachMultiEntityOnly)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 1;
    ecs.add<f32>(e1) = 10.0f;
    ecs.add<u32>(e2) = 2;
    ecs.add<f32>(e2) = 20.0f;

    u64 sum = 0;
    ecs.forEach<u32, f32>([&](Entity e)
    {
        sum += ecs.get<u32>(e);
    });
    ASSERT(sum == 3);
}

TEST(testForEachMultiRefOnly)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 1;
    ecs.add<f32>(e1) = 10.0f;
    ecs.add<u32>(e2) = 2;
    ecs.add<f32>(e2) = 20.0f;

    u64 sum = 0;
    ecs.forEach<u32, f32>([&](u32& a, f32& b)
    {
        sum += a + (u64)b;
    });
    ASSERT(sum == 33);
}

TEST(testForEachParMultiEntityAndRef)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 1;
    ecs.add<f32>(e1) = 10.0f;
    ecs.add<u32>(e2) = 2;
    ecs.add<f32>(e2) = 20.0f;

    SpinLock lk;
    u64 sum = 0;
    ecs.forEachPar<u32, f32>([&](Entity e, u32& a, f32& b)
    {
        SpinLockScope scope{lk};
        sum += a + (u64)b;
        (void)e;
    });
    ASSERT(sum == 33);
}

TEST(testForEachParMultiEntityOnly)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 1;
    ecs.add<f32>(e1) = 10.0f;
    ecs.add<u32>(e2) = 2;
    ecs.add<f32>(e2) = 20.0f;

    SpinLock lk;
    u64 sum = 0;
    ecs.forEachPar<u32, f32>([&](Entity e)
    {
        SpinLockScope scope{lk};
        sum += ecs.get<u32>(e);
    });
    ASSERT(sum == 3);
}

TEST(testForEachParMultiRefOnly)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 1;
    ecs.add<f32>(e1) = 10.0f;
    ecs.add<u32>(e2) = 2;
    ecs.add<f32>(e2) = 20.0f;

    SpinLock lk;
    u64 sum = 0;
    ecs.forEachPar<u32, f32>([&](u32& a, f32& b)
    {
        SpinLockScope scope{lk};
        sum += a + (u64)b;
    });
    ASSERT(sum == 33);
}

TEST(testForEachMultiOnlyMatchingEntities)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    Entity e3 = ecs.spawn();
    Entity e4 = ecs.spawn();
    ecs.add<u32>(e1) = 1;
    ecs.add<u32>(e2) = 2;
    ecs.add<f32>(e2) = 20.0f;
    ecs.add<f32>(e3) = 30.0f;
    ecs.add<u32>(e4) = 4;
    ecs.add<f32>(e4) = 40.0f;
    u64 visitCount = 0;
    u64 sum = 0;
    ecs.forEach<u32, f32>([&](Entity e, u32& a, f32& b)
    {
        visitCount++;
        sum += a + (u64)b;
        (void)e;
    });
    ASSERT(visitCount == 2);
    ASSERT(sum == (2 + 20) + (4 + 40));
}

TEST(testConstForEachSingleEntityAndRef)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<f32>(e1) = 1.0f;
    ecs.add<u32>(e2) = 20;
    ecs.add<f32>(e2) = 2.0f;
    const Ecs<u32, f32>& cecs = ecs;

    u64 sum = 0;
    cecs.forEach<u32>([&](Entity en, const u32& v)
    {
        sum += v;
        (void)en;
    });
    ASSERT(sum == 30);

    f32 fsum = 0;
    cecs.forEach<f32>([&](Entity en, const f32& v)
    {
        fsum += v;
        (void)en;
    });
    ASSERT(fsum == 3.0f);
}

TEST(testConstForEachSingleEntityOnly)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<u32>(e2) = 20;
    const Ecs<u32>& cecs = ecs;

    u64 sum = 0;
    cecs.forEach<u32>([&](Entity e)
    {
        sum += cecs.get<u32>(e);
    });
    ASSERT(sum == 30);
}

TEST(testConstForEachSingleRefOnly)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<u32>(e2) = 20;
    const Ecs<u32>& cecs = ecs;

    u64 sum = 0;
    cecs.forEach<u32>([&](const u32& v)
    {
        sum += v;
    });
    ASSERT(sum == 30);
}

TEST(testConstForEachMultiEntityAndRef)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 5;
    ecs.add<f32>(e1) = 10.0f;
    ecs.add<u32>(e2) = 15;
    ecs.add<f32>(e2) = 20.0f;
    const Ecs<u32, f32>& cecs = ecs;

    u64 sum = 0;
    cecs.forEach<u32, f32>([&](Entity en, const u32& a, const f32& b)
    {
        sum += a + (u64)b;
        (void)en;
    });
    ASSERT(sum == (5 + 10) + (15 + 20));
}

TEST(testConstForEachMultiEntityOnly)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 5;
    ecs.add<f32>(e1) = 10.0f;
    ecs.add<u32>(e2) = 15;
    ecs.add<f32>(e2) = 20.0f;
    const Ecs<u32, f32>& cecs = ecs;

    u64 sum = 0;
    cecs.forEach<u32, f32>([&](Entity e)
    {
        sum += cecs.get<u32>(e);
    });
    ASSERT(sum == 20);
}

TEST(testConstForEachMultiRefOnly)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 5;
    ecs.add<f32>(e1) = 10.0f;
    ecs.add<u32>(e2) = 15;
    ecs.add<f32>(e2) = 20.0f;
    const Ecs<u32, f32>& cecs = ecs;

    u64 sum = 0;
    cecs.forEach<u32, f32>([&](const u32& a, const f32& b)
    {
        sum += a + (u64)b;
    });
    ASSERT(sum == (5 + 10) + (15 + 20));
}
