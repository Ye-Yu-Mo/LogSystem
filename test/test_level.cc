// test_level.cc —— LogLevel toString/fromString 测试
#include <gtest/gtest.h>
#include "../logs/level.hpp"

using Xulog::LogLevel;

// toString 覆盖每个有效枚举
TEST(LevelTest, ToStringAllLevels)
{
    EXPECT_STREQ("DEBUG", LogLevel::toString(LogLevel::value::DEBUG));
    EXPECT_STREQ("INFO", LogLevel::toString(LogLevel::value::INFO));
    EXPECT_STREQ("WARN", LogLevel::toString(LogLevel::value::WARN));
    EXPECT_STREQ("ERROR", LogLevel::toString(LogLevel::value::ERROR));
    EXPECT_STREQ("FATAL", LogLevel::toString(LogLevel::value::FATAL));
    EXPECT_STREQ("OFF", LogLevel::toString(LogLevel::value::OFF));
}

// UNKNOW 及非法值落到 default 分支
TEST(LevelTest, ToStringUnknown)
{
    EXPECT_STREQ("UNKNOW", LogLevel::toString(LogLevel::value::UNKNOW));
}

// fromString 覆盖每个有效字符串
TEST(LevelTest, FromStringAllLevels)
{
    EXPECT_EQ(LogLevel::value::DEBUG, LogLevel::fromString("DEBUG"));
    EXPECT_EQ(LogLevel::value::INFO, LogLevel::fromString("INFO"));
    EXPECT_EQ(LogLevel::value::WARN, LogLevel::fromString("WARN"));
    EXPECT_EQ(LogLevel::value::ERROR, LogLevel::fromString("ERROR"));
    EXPECT_EQ(LogLevel::value::FATAL, LogLevel::fromString("FATAL"));
    EXPECT_EQ(LogLevel::value::OFF, LogLevel::fromString("OFF"));
}

// 非法字符串返回 UNKNOW
TEST(LevelTest, FromStringInvalid)
{
    EXPECT_EQ(LogLevel::value::UNKNOW, LogLevel::fromString("debug")); // 大小写敏感
    EXPECT_EQ(LogLevel::value::UNKNOW, LogLevel::fromString(""));
    EXPECT_EQ(LogLevel::value::UNKNOW, LogLevel::fromString("NOSUCH"));
}

// 往返一致性：toString -> fromString 应回到原值（UNKNOW 除外，它无对应字符串）
TEST(LevelTest, RoundTrip)
{
    for (auto lv : {LogLevel::value::DEBUG, LogLevel::value::INFO,
                    LogLevel::value::WARN, LogLevel::value::ERROR,
                    LogLevel::value::FATAL, LogLevel::value::OFF})
    {
        EXPECT_EQ(lv, LogLevel::fromString(LogLevel::toString(lv)));
    }
}
