/*
    日志等级类的实现
        1. 定义枚举类，枚举出日志的等级
        2. 提供转换接口，将枚举转换成对应的字符串
*/
#pragma once

namespace Xulog
{
    class LogLevel
    {
    public:
        enum class value
        {
            UNKNOW = 0,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            OFF
        };
        static const char *toString(LogLevel::value level)
        {
            switch (level)
            {
            case LogLevel::value::DEBUG:
                return "DEBUG";
                break;
            case LogLevel::value::INFO:
                return "INFO";
                break;
            case LogLevel::value::WARN:
                return "WARN";
                break;
            case LogLevel::value::ERROR:
                return "ERROR";
                break;
            case LogLevel::value::FATAL:
                return "FATAL";
                break;
            case LogLevel::value::OFF:
                return "OFF";
                break;
            }
            return "UNKNOW";
        }
    };
}