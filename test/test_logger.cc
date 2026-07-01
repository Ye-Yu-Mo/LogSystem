// test_logger.cc —— SyncLogger 多线程并发写，验证 M1 _msg 修复
#include <gtest/gtest.h>
#include "../logs/logger.hpp"
#include "../logs/sink.hpp"
#include <thread>
#include <vector>
#include <mutex>
#include <sstream>
#include <set>

// ---- 收集型 Sink：线程安全地把每条 LogMsg 存起来 ----------------
class CaptureSink : public Xulog::LogSink
{
public:
    void log(const char *data, size_t len) override
    {
        std::lock_guard<std::mutex> lk(_mu);
        _lines.emplace_back(data, len);
    }
    void log(const char *data, size_t len, const Xulog::LogMsg &msg) override
    {
        std::lock_guard<std::mutex> lk(_mu);
        _msgs.push_back(msg);
        _lines.emplace_back(data, len);
    }
    std::vector<std::string> lines() const { return _lines; }
    std::vector<Xulog::LogMsg> msgs() const { return _msgs; }

private:
    std::mutex _mu;
    std::vector<std::string> _lines;
    std::vector<Xulog::LogMsg> _msgs;
};

// ---- 辅助：创建带 CaptureSink 的 SyncLogger -------------------
static Xulog::Logger::ptr makeSyncLogger(const std::string &name,
                                          std::shared_ptr<CaptureSink> sink)
{
    auto formatter = std::make_shared<Xulog::Formatter>("%p|%m");
    std::vector<Xulog::LogSink::ptr> sinks{sink};
    return std::make_shared<Xulog::SyncLogger>(name, Xulog::LogLevel::value::DEBUG, formatter, sinks);
}

// ----------------------------------------------------------------
// 单线程：5 个等级各写一条，全部落到 sink
TEST(SyncLoggerTest, AllLevelsSingleThread)
{
    auto sink = std::make_shared<CaptureSink>();
    auto logger = makeSyncLogger("test_single", sink);

    logger->debug("f.cc", 1, "d%d", 1);
    logger->info ("f.cc", 2, "i%d", 2);
    logger->warn ("f.cc", 3, "w%d", 3);
    logger->error("f.cc", 4, "e%d", 4);
    logger->fatal("f.cc", 5, "f%d", 5);

    ASSERT_EQ(5u, sink->lines().size());
    EXPECT_EQ("DEBUG|d1", sink->lines()[0]);
    EXPECT_EQ("INFO|i2",  sink->lines()[1]);
    EXPECT_EQ("WARN|w3",  sink->lines()[2]);
    EXPECT_EQ("ERROR|e4", sink->lines()[3]);
    EXPECT_EQ("FATAL|f5", sink->lines()[4]);
}

// 等级过滤：低于 WARN 的不输出
TEST(SyncLoggerTest, LevelFilter)
{
    auto formatter = std::make_shared<Xulog::Formatter>("%p|%m");
    auto sink = std::make_shared<CaptureSink>();
    std::vector<Xulog::LogSink::ptr> sinks{sink};
    Xulog::SyncLogger logger("test_filter", Xulog::LogLevel::value::WARN, formatter, sinks);

    logger.debug("f.cc", 1, "should be filtered");
    logger.info ("f.cc", 2, "should be filtered");
    logger.warn ("f.cc", 3, "pass");
    logger.error("f.cc", 4, "pass");

    ASSERT_EQ(2u, sink->lines().size());
    EXPECT_EQ("WARN|pass",  sink->lines()[0]);
    EXPECT_EQ("ERROR|pass", sink->lines()[1]);
}

// 多线程：4 线程各写 1000 条带唯一序号的日志
// 验证：总条数正确 + 每条 payload 完整（不截断/不交叉污染）
TEST(SyncLoggerTest, ConcurrentWriteNoCorruption)
{
    constexpr int THREADS = 4;
    constexpr int PER_THR = 1000;

    auto sink = std::make_shared<CaptureSink>();
    auto logger = makeSyncLogger("test_concurrent", sink);

    std::vector<std::thread> threads;
    for (int t = 0; t < THREADS; t++)
    {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < PER_THR; i++)
                logger->info("f.cc", 1, "t%d-i%d", t, i);
        });
    }
    for (auto &th : threads) th.join();

    ASSERT_EQ(THREADS * PER_THR, (int)sink->lines().size());

    // 每个线程的每个序号应该恰好出现一次
    std::set<std::string> seen;
    for (auto &line : sink->lines())
    {
        // 格式: "INFO|tX-iY"
        std::string payload = line.substr(5); // skip "INFO|"
        EXPECT_TRUE(seen.insert(payload).second) << "duplicate: " << payload;
    }
    EXPECT_EQ(THREADS * PER_THR, (int)seen.size());
}

// 多线程：同时验证结构化 LogMsg 的字段不错位（M1 病根修复验证）
TEST(SyncLoggerTest, ConcurrentMsgFieldsNotCorrupted)
{
    constexpr int THREADS = 4;
    constexpr int PER_THR = 500;

    auto sink = std::make_shared<CaptureSink>();
    auto logger = makeSyncLogger("test_fields", sink);

    std::vector<std::thread> threads;
    for (int t = 0; t < THREADS; t++)
    {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < PER_THR; i++)
                logger->info("f.cc", (size_t)(t * 10000 + i), "t%d-i%d", t, i);
        });
    }
    for (auto &th : threads) th.join();

    ASSERT_EQ(THREADS * PER_THR, (int)sink->msgs().size());

    // 每条 LogMsg 的 payload 和 line 字段应该互相对应（不错位）
    for (auto &msg : sink->msgs())
    {
        // payload 格式 "tT-iI"，line = T*10000+I
        int t_val, i_val;
        ASSERT_EQ(2, sscanf(msg._payload.c_str(), "t%d-i%d", &t_val, &i_val));
        size_t expected_line = (size_t)(t_val * 10000 + i_val);
        EXPECT_EQ(expected_line, msg._line)
            << "payload=" << msg._payload << " but line=" << msg._line;
    }
}
