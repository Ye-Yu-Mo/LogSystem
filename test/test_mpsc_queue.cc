// test_mpsc_queue.cc —— 无锁 MPSC 队列测试
#include <gtest/gtest.h>
#include "../logs/mpsc_queue.hpp"
#include <thread>
#include <vector>
#include <set>
#include <algorithm>
#include <chrono>

using Xulog::MpscQueue;

// ---- 基础功能：单生产者-单消费者 ----

TEST(MpscQueueTest, PushPopSingleThread)
{
    MpscQueue<int> q(100, false); // UNSAFE, no limit
    q.push(1);
    q.push(2);
    q.push(3);

    auto batch = q.popAll();
    ASSERT_EQ(3u, batch.size());
    EXPECT_EQ(1, batch[0]);
    EXPECT_EQ(2, batch[1]);
    EXPECT_EQ(3, batch[2]);
}

TEST(MpscQueueTest, EmptyQueuePopAll)
{
    MpscQueue<int> q(100, false);
    EXPECT_TRUE(q.empty());
    auto batch = q.popAll();
    EXPECT_TRUE(batch.empty());
}

TEST(MpscQueueTest, PopAllClearsQueue)
{
    MpscQueue<int> q(100, false);
    q.push(42);
    auto batch = q.popAll();
    ASSERT_EQ(1u, batch.size());
    EXPECT_TRUE(q.empty());
    auto second = q.popAll();
    EXPECT_TRUE(second.empty());
}

// ---- FIFO 顺序验证 ----

TEST(MpscQueueTest, PreservesFifoOrder)
{
    MpscQueue<int> q(10000, false);
    constexpr int N = 5000;
    for (int i = 0; i < N; i++)
        q.push(i);

    auto batch = q.popAll();
    ASSERT_EQ(N, (int)batch.size());
    for (int i = 0; i < N; i++)
        EXPECT_EQ(i, batch[i]) << "order broken at index " << i;
}

// ---- 多生产者并发：不丢不重 ----

TEST(MpscQueueTest, MultiProducerNoLoss)
{
    constexpr int THREADS = 4;
    constexpr int PER_THR = 5000;

    MpscQueue<int> q(THREADS * PER_THR * 2, true); // SAFE, big enough

    std::vector<std::thread> producers;
    for (int t = 0; t < THREADS; t++)
    {
        producers.emplace_back([&q, t]() {
            for (int i = 0; i < PER_THR; i++)
                q.push(t * 100000 + i);
        });
    }
    for (auto &th : producers)
        th.join();

    auto batch = q.popAll();
    ASSERT_EQ(THREADS * PER_THR, (int)batch.size());

    // 每个唯一值应该出现恰好一次
    std::set<int> seen(batch.begin(), batch.end());
    EXPECT_EQ(THREADS * PER_THR, (int)seen.size());
}

// ---- SAFE 模式背压 ----

TEST(MpscQueueTest, SafeModeBackpressure)
{
    constexpr size_t MAX = 50;
    MpscQueue<int> q(MAX, true); // SAFE, small limit

    // 生产者线程：push 超过上限，内部会 yield 等待
    std::atomic<bool> done{false};
    std::atomic<size_t> pushed{0};

    std::thread producer([&]() {
        for (int i = 0; i < 200; i++)
        {
            q.push(i);
            pushed.fetch_add(1);
        }
        done.store(true);
    });

    // 消费者慢速取出，模拟消费跟不上
    size_t total_consumed = 0;
    while (!done.load() || !q.empty())
    {
        auto batch = q.popAll();
        total_consumed += batch.size();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    producer.join();

    EXPECT_EQ(200u, pushed.load());          // 全部成功入队
    EXPECT_EQ(200u, total_consumed);         // 全部成功消费
    EXPECT_LE(pushed.load(), (size_t)200);   // 背压没丢消息
}

// ---- UNSAFE 模式不设上限 ----

TEST(MpscQueueTest, UnsafeNoLimit)
{
    MpscQueue<int> q(10, false); // UNSAFE, limit ignored
    // 推入远超 "max" 的值，UNSAFE 应全部接受
    for (int i = 0; i < 500; i++)
        q.push(i);

    auto batch = q.popAll();
    EXPECT_EQ(500u, batch.size());
}

// ---- 多生产者 + 消费者交替进行 ----

TEST(MpscQueueTest, ConcurrentPushAndPop)
{
    constexpr int THREADS = 4;
    constexpr int ROUNDS = 10;
    constexpr int PER_ROUND = 1000;

    MpscQueue<int> q(THREADS * PER_ROUND * 2, true);
    std::atomic<size_t> total_pushed{0};
    std::atomic<size_t> total_popped{0};
    std::atomic<bool> stop{false};

    // 消费者线程
    std::thread consumer([&]() {
        while (!stop.load() || !q.empty())
        {
            auto batch = q.popAll();
            total_popped.fetch_add(batch.size());
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        // final drain
        auto batch = q.popAll();
        total_popped.fetch_add(batch.size());
    });

    // 多轮生产
    for (int r = 0; r < ROUNDS; r++)
    {
        std::vector<std::thread> producers;
        for (int t = 0; t < THREADS; t++)
        {
            producers.emplace_back([&q, &total_pushed, t, r]() {
                for (int i = 0; i < PER_ROUND; i++)
                {
                    q.push(t * 100000 + r * 10000 + i);
                    total_pushed.fetch_add(1);
                }
            });
        }
        for (auto &th : producers)
            th.join();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    stop.store(true);
    consumer.join();

    EXPECT_EQ(total_pushed.load(), total_popped.load());
    EXPECT_EQ(THREADS * ROUNDS * PER_ROUND, (int)total_pushed.load());
}

// ---- count/hasData 辅助方法 ----

TEST(MpscQueueTest, CountAndHasData)
{
    MpscQueue<int> q(100, true);
    EXPECT_FALSE(q.hasData());
    EXPECT_EQ(0u, q.count());

    q.push(1);
    EXPECT_TRUE(q.hasData());
    EXPECT_EQ(1u, q.count());

    q.push(2);
    EXPECT_EQ(2u, q.count());

    auto batch = q.popAll();
    EXPECT_FALSE(q.hasData());
    EXPECT_EQ(0u, q.count());
    EXPECT_EQ(2u, batch.size());
}
