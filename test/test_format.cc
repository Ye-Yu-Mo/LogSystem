// test_format.cc —— Formatter 占位符输出 + 非法格式串异常
#include <gtest/gtest.h>
#include "../logs/format.hpp"
#include "../logs/message.hpp"
#include "../logs/level.hpp"
#include <thread>
#include <regex>

using Xulog::Formatter;
using Xulog::LogMsg;
using Xulog::LogLevel;

// 辅助：构造一条固定内容的 LogMsg
static LogMsg makeMsg(LogLevel::value lv = LogLevel::value::INFO,
                      const std::string &file = "test.cc",
                      size_t line = 42,
                      const std::string &logger = "root",
                      const std::string &payload = "hello world")
{
    return LogMsg(lv, line, file, logger, payload);
}

// %m —— 消息正文
TEST(FormatterTest, MsgPayload)
{
    Formatter fmt("%m");
    auto msg = makeMsg(LogLevel::value::INFO, "f.cc", 1, "lg", "test payload");
    EXPECT_EQ("test payload", fmt.Format(msg));
}

// %p —— 日志等级字符串
TEST(FormatterTest, LevelPlaceholder)
{
    Formatter fmt("%p");
    for (auto [lv, str] : std::initializer_list<std::pair<LogLevel::value, const char *>>{
             {LogLevel::value::DEBUG, "DEBUG"},
             {LogLevel::value::INFO, "INFO"},
             {LogLevel::value::WARN, "WARN"},
             {LogLevel::value::ERROR, "ERROR"},
             {LogLevel::value::FATAL, "FATAL"},
         })
    {
        auto msg = makeMsg(lv);
        EXPECT_EQ(str, fmt.Format(msg));
    }
}

// %f —— 文件名
TEST(FormatterTest, FilePlaceholder)
{
    Formatter fmt("%f");
    auto msg = makeMsg(LogLevel::value::INFO, "myfile.cc");
    EXPECT_EQ("myfile.cc", fmt.Format(msg));
}

// %l —— 行号
TEST(FormatterTest, LinePlaceholder)
{
    Formatter fmt("%l");
    auto msg = makeMsg(LogLevel::value::INFO, "x.cc", 99);
    EXPECT_EQ("99", fmt.Format(msg));
}

// %c —— 日志器名称
TEST(FormatterTest, LoggerNamePlaceholder)
{
    Formatter fmt("%c");
    auto msg = makeMsg(LogLevel::value::INFO, "x.cc", 1, "mylogger");
    EXPECT_EQ("mylogger", fmt.Format(msg));
}

// %T —— 制表符
TEST(FormatterTest, TabPlaceholder)
{
    Formatter fmt("%T");
    auto msg = makeMsg();
    EXPECT_EQ("\t", fmt.Format(msg));
}

// %n —— 换行符
TEST(FormatterTest, NewlinePlaceholder)
{
    Formatter fmt("%n");
    auto msg = makeMsg();
    EXPECT_EQ("\n", fmt.Format(msg));
}

// %% —— 字面 %
TEST(FormatterTest, EscapedPercent)
{
    Formatter fmt("%%");
    auto msg = makeMsg();
    EXPECT_EQ("%", fmt.Format(msg));
}

// 原始字符串原样保留
TEST(FormatterTest, LiteralString)
{
    Formatter fmt("[literal]");
    auto msg = makeMsg();
    EXPECT_EQ("[literal]", fmt.Format(msg));
}

// %d 时间格式化：结果应匹配给定的时间模式，不为空
TEST(FormatterTest, DatePlaceholderNotEmpty)
{
    Formatter fmt("%d{%H:%M:%S}");
    auto msg = makeMsg();
    std::string result = fmt.Format(msg);
    // HH:MM:SS，简单验证格式 \d\d:\d\d:\d\d
    EXPECT_TRUE(std::regex_match(result, std::regex(R"(\d{2}:\d{2}:\d{2})")))
        << "actual: " << result;
}

// %t —— 线程 ID 不为空
TEST(FormatterTest, ThreadIdNotEmpty)
{
    Formatter fmt("%t");
    auto msg = makeMsg();
    EXPECT_FALSE(fmt.Format(msg).empty());
}

// 组合格式：常见格式串能正确拼出各段
TEST(FormatterTest, CombinedFormat)
{
    Formatter fmt("[%p]%T%m%n");
    auto msg = makeMsg(LogLevel::value::WARN, "f.cc", 1, "lg", "body");
    EXPECT_EQ("[WARN]\tbody\n", fmt.Format(msg));
}

// getPattern 返回构造时传入的格式串
TEST(FormatterTest, GetPattern)
{
    const std::string pat = "%p %m%n";
    Formatter fmt(pat);
    EXPECT_EQ(pat, fmt.getPattern());
}

// 非法格式字符 —— 应抛 std::invalid_argument（M1 修复点）
TEST(FormatterTest, InvalidFormatCharThrows)
{
    EXPECT_THROW(Formatter fmt("%z"), std::invalid_argument);
}

// 未闭合的 {} —— 解析失败应抛异常
TEST(FormatterTest, UnclosedBraceThrows)
{
    EXPECT_THROW(Formatter fmt("%d{%H:%M:%S"), std::invalid_argument);
}
