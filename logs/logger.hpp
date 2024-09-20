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
#include "looper.hpp"
#include <atomic>
#include <mutex>
#include <cstdarg>
#include <unordered_map>

namespace Xulog
{
    class Logger
    {
    public:
        using ptr = std::shared_ptr<Logger>;
        Logger(const std::string &loggername,
               LogLevel::value level,
               Formatter::ptr &formatter,
               std::vector<LogSink::ptr> sinks)
            : _logger_name(loggername),
              _limit_level(level),
              _formatter(formatter),
              _sinks(sinks.begin(),sinks.end())
        {
        }
        const std::string &name()
        {
            return _logger_name;
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
    // DONE 异步日志器
    class AsyncLogger : public Logger
    {
    public:
        AsyncLogger(const std::string &loggername,
                    LogLevel::value level,
                    Formatter::ptr &formatter,
                    std::vector<LogSink::ptr> sinks,
                    AsyncType looper_type)
            : Logger(loggername, level, formatter, sinks),
              _looper(std::make_shared<AsyncLooper>(std::bind(&AsyncLogger::realLog, this, std::placeholders::_1), looper_type))
        {
        }

        void log(const char *data, size_t len) // 将数据写入缓冲区
        {
            _looper->push(data, len);
        }
        // 实际落地函数，将缓冲区中的函数落地
        void realLog(Buffer &buf)
        {
            if (_sinks.empty())
                return;
            for (auto &sink : _sinks)
            {
                sink->log(buf.begin(), buf.readAbleSize());
            }
        }

    private:
        AsyncLooper::ptr _looper;
    };

    // 使用建造者模式建造日志器，无需用户构造
    enum class LoggerType
    {
        LOGGER_SYNC,
        LOGGER_ASYNC
    };
    class LoggerBuilder
    {
    public:
        LoggerBuilder() : _logger_type(LoggerType::LOGGER_SYNC),
                          _limit_level(LogLevel::value::DEBUG),
                          _looper_type(AsyncType::ASYNC_SAFE)
        {
        }
        void buildEnableUnsafeAsync()
        {
            _looper_type = AsyncType::ASYNC_UNSAFE;
        }
        void buildLoggerType(LoggerType type)
        {
            _logger_type = type;
        }
        void buildLoggerName(const std::string &name)
        {
            _logger_name = name;
        }
        void buildLoggerLevel(LogLevel::value level)
        {
            _limit_level = level;
        }
        void buildFormatter(const std::string &pattern = "[%d{%y-%m-%d|%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n")
        {
            _formatter = std::make_shared<Formatter>(pattern);
        }
        template <typename SinkType, typename... Args>
        void buildSink(Args &&...args)
        {
            LogSink::ptr psink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
            _sinks.push_back(psink);
        }
        virtual Logger::ptr build() = 0;

    protected:
        AsyncType _looper_type;
        LoggerType _logger_type;
        std::string _logger_name;
        LogLevel::value _limit_level;
        Formatter::ptr _formatter;
        std::vector<LogSink::ptr> _sinks;
    };

    // 局部日志器建造者
    class LocalLoggerBuild : public LoggerBuilder
    {
    public:
        Logger::ptr build() override
        {
            assert(!_logger_name.empty());
            if (_sinks.empty())
            {
                buildSink<StdoutSink>();
            }                
            if (_logger_type == LoggerType::LOGGER_ASYNC)
            {
                return std::make_shared<AsyncLogger>(_logger_name, _limit_level, _formatter, _sinks, _looper_type);
            }
            return std::make_shared<SyncLogger>(_logger_name, _limit_level, _formatter, _sinks);
        }
    };
    // 日志器管理器
    // 懒汉单例模式
    class LoggerManager
    {
    public:
        static LoggerManager &getInstance()
        {
            // C++11 之后，静态局部变量是线程安全的

            static LoggerManager eton;
            return eton;
        }
        void addLogger(Logger::ptr &logger)
        {
            // 不加锁的情况下先检查
            if (hasLogger(logger->name()))
                return;
            // 加锁后直接插入 避免重复加锁
            std::unique_lock<std::mutex> lock(_mutex);
            _loggers.insert(std::make_pair(logger->name(), logger));
        }

        bool hasLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if (it == _loggers.end())
                return false;
            return true;
        }
        Logger::ptr getLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if (it == _loggers.end())
                return Logger::ptr();
            return it->second;
        }
        Logger::ptr rootLogger()
        {
            return _root_logger;
        }

    private:
        LoggerManager()
        {
            std::unique_ptr<Xulog::LoggerBuilder> builder(new Xulog::LocalLoggerBuild());
            builder->buildLoggerName("root");
            builder->buildFormatter();
            _root_logger = builder->build();
            _loggers.insert(std::make_pair("root", _root_logger));
        }
        ~LoggerManager() {}

    private:
        std::mutex _mutex;
        Logger::ptr _root_logger; // 默认日志器
        std::unordered_map<std::string, Logger::ptr> _loggers;
    };

    // DONE 全局日志器建造者
    class GlobalLoggerBuild : public LoggerBuilder
    {
    public:
        Logger::ptr build() override
        {
            assert(!_logger_name.empty());
            if (_sinks.empty())
            {
                buildSink<StdoutSink>();
            }
            Logger::ptr logger;
            if (_logger_type == LoggerType::LOGGER_ASYNC)
            {
                logger = std::make_shared<AsyncLogger>(_logger_name, _limit_level, _formatter, _sinks, _looper_type);
            }
            else
            {
                logger = std::make_shared<SyncLogger>(_logger_name, _limit_level, _formatter, _sinks);
            }
            
            LoggerManager::getInstance().addLogger(logger);
            return logger;
        }
    };

}
