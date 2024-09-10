/*
    日志器模块的实现
        1. 抽象日志器基类
        2. 派生同步异步日志器子类
*/
#pragma once
// #ifndef _GUN_SOURCE
// #define _GNU_SOURCE
// #endif
#include "util.hpp"
#include "level.hpp"
#include "format.hpp"
#include "sink.hpp"
#include <atomic>
#include <mutex>
#include <cstdarg>

namespace Xulog
{
    class Logger
    {
    public:
        using ptr = std::shared_ptr<Logger>;
        Logger(const std::string &loggername, LogLevel::value level, Formatter::ptr &formatter, std::vector<LogSink::ptr> sinks)
            : _logger_name(loggername), _limit_level(level), _formatter(formatter), _sinks(sinks.begin(), sinks.end())
        {
        }
        // 构造日志消息，格式化，交给输出接口
        void debug(const std::string &file, size_t line, const std::string &fmt, ...)
        {
            // 是否达到输出等级
            if (LogLevel::value::DEBUG < _limit_level)
                return;
            // 将不定参进行格式化，将其转为字符串
            va_list ap;
            va_start(ap, fmt);
            char *res;
            int ret = vasprintf(&res, fmt.c_str(), ap);
            if (ret == -1)
            {
                std::cout << "vasprintf fail\n";
                return;
            }
            va_end(ap);
            serialize(LogLevel::value::DEBUG, file, line, res);
            free(res);
        }
        void info(const std::string &file, size_t line, const std::string &fmt, ...)
        {
            // 是否达到输出等级
            if (LogLevel::value::INFO < _limit_level)
                return;
            // 将不定参进行格式化，将其转为字符串
            va_list ap;
            va_start(ap, fmt);
            char *res;
            int ret = vasprintf(&res, fmt.c_str(), ap);
            if (ret == -1)
            {
                std::cout << "vasprintf fail\n";
                return;
            }
            va_end(ap);
            serialize(LogLevel::value::INFO, file, line, res);
            free(res);
        }
        void warn(const std::string &file, size_t line, const std::string &fmt, ...)
        {
            // 是否达到输出等级
            if (LogLevel::value::WARN < _limit_level)
                return;
            // 将不定参进行格式化，将其转为字符串
            va_list ap;
            va_start(ap, fmt);
            char *res;
            int ret = vasprintf(&res, fmt.c_str(), ap);
            if (ret == -1)
            {
                std::cout << "vasprintf fail\n";
                return;
            }
            va_end(ap);
            serialize(LogLevel::value::WARN, file, line, res);
            free(res);
        }
        void error(const std::string &file, size_t line, const std::string &fmt, ...)
        {
            // 是否达到输出等级
            if (LogLevel::value::ERROR < _limit_level)
                return;
            // 将不定参进行格式化，将其转为字符串
            va_list ap;
            va_start(ap, fmt);
            char *res;
            int ret = vasprintf(&res, fmt.c_str(), ap);
            if (ret == -1)
            {
                std::cout << "vasprintf fail\n";
                return;
            }
            va_end(ap);
            serialize(LogLevel::value::ERROR, file, line, res);
            free(res);
        }
        void fatal(const std::string &file, size_t line, const std::string &fmt, ...)
        {
            // 是否达到输出等级
            if (LogLevel::value::FATAL < _limit_level)
                return;
            // 将不定参进行格式化，将其转为字符串
            va_list ap;
            va_start(ap, fmt);
            char *res;
            int ret = vasprintf(&res, fmt.c_str(), ap);
            if (ret == -1)
            {
                std::cout << "vasprintf fail\n";
                return;
            }
            va_end(ap);
            serialize(LogLevel::value::FATAL, file, line, res);
            free(res);
        }

    protected:
        // 抽象接口完成实际的落地输出，同步异步的落地方式不同
        virtual void log(const char *data, size_t len) = 0;
        void serialize(LogLevel::value level, const std::string &file, size_t line, char *str)
        {
            // 日志内容格式化
            LogMsg msg(level, line, file, _logger_name, str);
            std::stringstream ss;
            _formatter->Format(ss, msg);
            // 日志落地
            log(ss.str().c_str(), ss.str().size());
        }

        std::mutex _mutex;
        std::string _logger_name;
        std::atomic<LogLevel::value> _limit_level;
        Formatter::ptr _formatter;
        std::vector<LogSink::ptr> _sinks;
    };
    // 同步日志器
    class SyncLogger : public Logger
    {
    public:
        SyncLogger(const std::string &loggername, LogLevel::value level, Formatter::ptr &formatter, std::vector<LogSink::ptr> sinks)
            : Logger(loggername, level, formatter, sinks)
        {
        }

    protected:
        // 直接通过落地模块句柄进行日志输出
        void log(const char *data, size_t len) override
        {
            std::unique_lock<std::mutex> lock(_mutex);
            if (_sinks.empty())
                return;
            else
                for (auto &sink : _sinks)
                {
                    sink->log(data, len);
                }
        }
    };
    // TODO 异步日志器
}
