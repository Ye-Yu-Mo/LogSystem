/**
 * @file logger.hpp
 * @brief 日志器模块的实现
 *
 * 该模块包含日志器的抽象基类及其派生类，实现了同步和异步日志记录功能。
 *
 * @section overview 概述
 * - 抽象日志器基类
 * - 派生同步日志器和异步日志器子类
 */
#pragma once
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
    /**
     * @class Logger
     * @brief 抽象日志器基类
     *
     * Logger 类用于记录日志，提供多种日志级别的方法，并将日志格式化后输出到不同的接收器。
     */
    class Logger
    {
    public:
        using ptr = std::shared_ptr<Logger>;
        /**
         * @brief 构造函数
         *
         * @param loggername 日志器名称
         * @param level 日志级别
         * @param formatter 日志格式化器
         * @param sinks 日志输出接收器
         */
        Logger(const std::string &loggername,
               LogLevel::value level,
               Formatter::ptr &formatter,
               std::vector<LogSink::ptr> sinks)
            : _logger_name(loggername),
              _limit_level(level),
              _formatter(formatter),
              _sinks(sinks.begin(), sinks.end()) {}
        /**
         * @brief 获取日志器名称
         *
         * @return 日志器名称
         */
        const std::string &name()
        {
            return _logger_name;
        }
        /**
         * @brief 记录调试级别日志
         *
         * @param file 文件名
         * @param line 行号
         * @param fmt 格式化字符串
         */
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
        /**
         * @brief 记录信息级别日志
         *
         * @param file 文件名
         * @param line 行号
         * @param fmt 格式化字符串
         */
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
        /**
         * @brief 记录警告级别日志
         *
         * @param file 文件名
         * @param line 行号
         * @param fmt 格式化字符串
         */
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
        /**
         * @brief 记录错误级别日志
         *
         * @param file 文件名
         * @param line 行号
         * @param fmt 格式化字符串
         */

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
        /**
         * @brief 记录致命级别日志
         *
         * @param file 文件名
         * @param line 行号
         * @param fmt 格式化字符串
         */
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
        /// @brief 获取日志消息的结构化数据
        /// @return 日志消息结构体
        LogMsg getMsg()
        {
            return _msg;
        }
    protected:
        /**
         * @brief 抽象日志输出接口
         *
         * @param data 日志数据
         * @param len 数据长度
         */
        virtual void log(const char *data, size_t len) = 0;
        /**
         * @brief 序列化日志消息
         *
         * @param level 日志级别
         * @param file 文件名
         * @param line 行号
         * @param str 格式化后的日志字符串
         */
        void serialize(LogLevel::value level, const std::string &file, size_t line, char *str)
        {
            // 日志内容格式化
            _msg = LogMsg(level, line, file, _logger_name, str);

            std::stringstream ss;
            _formatter->Format(ss, _msg);
            // 日志落地
            log(ss.str().c_str(), ss.str().size());
        }
        LogMsg _msg;                               ///< 存储临时的消息对象
        std::mutex _mutex;                         ///< 互斥锁
        std::string _logger_name;                  ///< 日志器名称
        std::atomic<LogLevel::value> _limit_level; ///< 日志级别
        Formatter::ptr _formatter;                 ///< 日志格式化器
        std::vector<LogSink::ptr> _sinks;          ///< 日志输出接收器
    };
    /**
     * @class SyncLogger
     * @brief 同步日志器
     *
     * SyncLogger 实现了同步的日志记录功能，直接输出日志到接收器。
     */
    class SyncLogger : public Logger
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param loggername 日志器名称
         * @param level 日志级别
         * @param formatter 日志格式化器
         * @param sinks 日志输出接收器
         */
        SyncLogger(const std::string &loggername, LogLevel::value level, Formatter::ptr &formatter, std::vector<LogSink::ptr> sinks)
            : Logger(loggername, level, formatter, sinks)
        {
        }

    protected:
        /**
         * @brief 直接通过落地模块句柄进行日志输出
         *
         * @param data 日志数据
         * @param len 数据长度
         */
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
    /**
     * @class AsyncLogger
     * @brief 异步日志器
     *
     * AsyncLogger 实现了异步的日志记录功能，使用缓冲区和事件循环。
     */
    class AsyncLogger : public Logger
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param loggername 日志器名称
         * @param level 日志级别
         * @param formatter 日志格式化器
         * @param sinks 日志输出接收器
         * @param looper_type 异步类型
         */
        AsyncLogger(const std::string &loggername,
                    LogLevel::value level,
                    Formatter::ptr &formatter,
                    std::vector<LogSink::ptr> sinks,
                    AsyncType looper_type)
            : Logger(loggername, level, formatter, sinks),
              _looper(std::make_shared<AsyncLooper>(std::bind(&AsyncLogger::realLog, this, std::placeholders::_1), looper_type))
        {
        }
        /**
         * @brief 将数据写入缓冲区
         *
         * @param data 日志数据
         * @param len 数据长度
         */
        void log(const char *data, size_t len) // 将数据写入缓冲区
        {
            _looper->push(data, len);
        }
        /**
         * @brief 实际落地函数，将缓冲区中的日志写入接收器
         *
         * @param buf 日志缓冲区
         */
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
        AsyncLooper::ptr _looper; ///< 异步事件循环器
    };

    /**
     * @enum LoggerType
     * @brief 日志器类型
     *
     * 定义了同步和异步日志器的类型。
     */
    enum class LoggerType
    {
        LOGGER_SYNC, ///< 同步日志器
        LOGGER_ASYNC ///< 异步日志器
    };
    /**
     * @class LoggerBuilder
     * @brief 日志器建造者
     *
     * 使用建造者模式构建日志器，无需用户直接构造。
     */
    class LoggerBuilder
    {
        using ptr = std::shared_ptr<LoggerBuilder>;

    public:
        /**
         * @brief 构建接收器
         *
         * @tparam SinkType 接收器类型
         * @tparam Args 构造参数
         * @param args 构造参数
         */
        LoggerBuilder() : _looper_type(AsyncType::ASYNC_SAFE),
                          _logger_type(LoggerType::LOGGER_SYNC),
                          _limit_level(LogLevel::value::DEBUG)

        {
        }
        /**
         * @brief @deprecated 启用不安全的异步日志记录
         *
         * 此方法设置日志器为不安全的异步模式。
         */
        void buildEnableUnsafeAsync()
        {
            _looper_type = AsyncType::ASYNC_UNSAFE;
        }
        /**
         * @brief 设置日志器类型
         *
         * @param type 日志器类型（同步或异步）
         * @note 默认为LOGGER_SYNC同步日志器
         */
        void buildLoggerType(LoggerType type)
        {
            _logger_type = type;
        }
        /**
         * @brief 设置日志器名称
         *
         * @param name 日志器的名称
         * @warning 日志器名称不允许为空!
         */
        void buildLoggerName(const std::string &name)
        {
            _logger_name = name;
        }
        /**
         * @brief 设置日志器级别
         *
         * @param level 日志级别
         * @note 默认为DEBUG等级
         */
        void buildLoggerLevel(LogLevel::value level)
        {
            _limit_level = level;
        }
        /**
         * @brief 设置日志格式
         *
         * @param pattern 日志输出的格式，默认为 "[%d{%y-%m-%d|%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n"
         * @note 格式说明
         * %d 日期，子格式{%H:%M:%S}
         * %T 缩进
         * %t 线程ID
         * %p 日志级别
         * %c 日志器名称
         * %f 文件名
         * %l 行号
         * %m 日志消息
         * %n 换行
         */
        void buildFormatter(const std::string &pattern = "[%d{%y-%m-%d|%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n")
        {
            _formatter = std::make_shared<Formatter>(pattern);
        }
        /**
         * @brief 构建接收器
         *
         * @tparam SinkType 接收器类型
         * @tparam Args 构造参数
         * @param args 构造参数
         */
        template <typename SinkType, typename... Args>
        void buildSink(Args &&...args)
        {
            LogSink::ptr psink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
            _sinks.push_back(psink);
        }
        /**
         * @brief 建造日志器
         *
         * @return 创建的 Logger 对象
         */
        virtual Logger::ptr build() = 0;

    protected:
        AsyncType _looper_type;           ///< 异步类型
        LoggerType _logger_type;          ///< 日志器类型
        std::string _logger_name;         ///< 日志器名称
        LogLevel::value _limit_level;     ///< 日志级别
        Formatter::ptr _formatter;        ///< 日志格式化器
        std::vector<LogSink::ptr> _sinks; ///< 日志输出接收器
    };

    /**
     * @class LocalLoggerBuild
     * @brief 局部日志器建造者
     *
     * 该类实现了局部日志器的构建逻辑。
     */
    class LocalLoggerBuild : public LoggerBuilder
    {
    public:
        /**
         * @brief 建造日志器
         *
         * @return 创建的 Logger 对象
         */
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
    /**
     * @class LoggerManager
     * @brief 日志器管理器
     *
     * 管理多个日志器的单例类，实现懒汉式单例模式。
     */
    class LoggerManager
    {
    public:
        /**
         * @brief 获取日志器管理器的实例
         *
         * @return 日志器管理器的单例引用
         *
         * 该方法使用静态局部变量实现线程安全的单例模式。
         */
        static LoggerManager &getInstance()
        {
            // C++11 之后，静态局部变量是线程安全的

            static LoggerManager eton;
            return eton;
        }
        /**
         * @brief 添加日志器到管理器
         *
         * @param logger 要添加的日志器
         *
         * 此方法首先检查日志器是否已经存在，如果不存在则添加。
         */
        void addLogger(Logger::ptr &logger)
        {
            // 不加锁的情况下先检查
            if (hasLogger(logger->name()))
                return;
            // 加锁后直接插入 避免重复加锁
            std::unique_lock<std::mutex> lock(_mutex);
            _loggers.insert(std::make_pair(logger->name(), logger));
        }
        /**
         * @brief 检查是否存在指定名称的日志器
         *
         * @param name 日志器名称
         * @return 如果存在返回 true，否则返回 false
         */
        bool hasLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if (it == _loggers.end())
                return false;
            return true;
        }
        /**
         * @brief 获取指定名称的日志器
         *
         * @param name 日志器名称
         * @return 找到的日志器，如果不存在返回 nullptr
         */
        Logger::ptr getLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if (it == _loggers.end())
                return Logger::ptr();
            return it->second;
        }
        /**
         * @brief 获取根日志器
         *
         * @return 根日志器的指针
         */
        Logger::ptr rootLogger()
        {
            return _root_logger;
        }

    private:
        /**
         * @brief 构造函数
         *
         * 初始化默认根日志器并将其添加到管理器中。
         */
        LoggerManager()
        {
            std::unique_ptr<Xulog::LoggerBuilder> builder(new Xulog::LocalLoggerBuild());
            builder->buildLoggerName("root");
            builder->buildFormatter();
            _root_logger = builder->build();
            _loggers.insert(std::make_pair("root", _root_logger));
        }
        /**
         * @brief 析构函数
         *
         * 私有析构函数，防止外部删除管理器实例。
         */
        ~LoggerManager() {}

    private:
        std::mutex _mutex;                                     ///< 互斥锁
        Logger::ptr _root_logger;                              ///< 默认日志器
        std::unordered_map<std::string, Logger::ptr> _loggers; ///< 日志器映射
    };

    /**
     * @class GlobalLoggerBuild
     * @brief 全局日志器建造者
     *
     * 使用全局设置构建日志器。
     */
    class GlobalLoggerBuild : public LoggerBuilder
    {
    public:
        /**
         * @brief 构建日志器
         *
         * @return 创建的 Logger 对象
         *
         * 根据设置创建同步或异步日志器，并将其添加到管理器中。
         */
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