/**
 * @file sink.hpp
 * @brief 日志落地模块的实现
 *
 * 本文件实现了日志落地的相关类，包括抽象基类 LogSink 及其派生类 StdoutSink、FileSink 和 RollSinkBySize。
 * 使用工厂模式进行日志输出对象的创建与管理。
 */
#pragma once
#include "util.hpp"
#include <memory>
#include <fstream>
#include <cassert>
#include <sstream>
#include <cstring>
#include <vector>
#include <utility>

namespace Xulog
{
    /**
     * @class LogSink
     * @brief 抽象日志落地基类
     *
     * 该类定义了日志输出的接口，所有日志落地实现都需要继承自此类。
     */
    class LogSink
    {
    public:
        using ptr = std::shared_ptr<LogSink>; /**< 智能指针类型 */
        LogSink() {}
        virtual ~LogSink() {}
        /**
         * @brief 日志输出
         *
         * @param data 日志数据
         * @param len 数据长度
         *
         * 纯虚函数，派生类必须实现此方法以完成日志的实际输出。
         */
        virtual void log(const char *data, size_t len) = 0;
    };
    /**
     * @class StdoutSink
     * @brief 标准输出日志落地实现
     *
     * 该类实现了将日志输出到标准输出的功能，并支持日志级别的颜色显示。
     */
    class StdoutSink : public LogSink
    {
    public:
        /**
         * @enum Color
         * @brief 日志颜色设置
         */
        enum class Color
        {
            Enable,
            Unenable
        };
        /**
         * @brief 构造函数
         *
         * @param enable 是否启用颜色显示
         */
        StdoutSink(Color enable = Color::Unenable)
            : _enable_color(enable)
        {
        }
        /**
         * @brief 日志写入到标准输出
         *
         * @param data 日志数据
         * @param len 数据长度
         *
         * 根据设置输出带颜色的日志。
         */
        void log(const char *data, size_t len)
        {
            // 设置日志等级颜色并输出
            if (_enable_color == Color::Enable)
            {
                std::string msg = setColorBasedOnLogLevel(data, len);
                std::cout << msg;
                return;
            }
            std::cout.write(data, len);
        }

    private:
        /**
         * @brief 根据日志级别设置颜色
         *
         * @param message 日志消息
         * @param len 消息长度
         * @return std::string 带颜色的日志消息
         */
        std::string setColorBasedOnLogLevel(const char *message, size_t len)
        {
            std::string logMessage(message, len);

            // 存储日志级别和对应颜色
            std::vector<std::pair<std::string, std::string>> logLevels = {
                {"DEBUG", COLOR_DEBUG + "DEBUG" + COLOR_RESET},
                {"INFO", COLOR_INFO + "INFO" + COLOR_RESET},
                {"WARN", COLOR_WARN + "WARN" + COLOR_RESET},
                {"ERROR", COLOR_ERROR + "ERROR" + COLOR_RESET},
                {"FATAL", COLOR_FATAL + "FATAL" + COLOR_RESET}};

            for (auto &level : logLevels)
            {
                size_t pos = 0;
                while (pos < logMessage.size())
                {
                    size_t found = logMessage.find(level.first, pos);
                    if (found == std::string::npos)
                        break;
                    logMessage.replace(found, level.first.size(), level.second);
                    pos = found + level.second.size();
                }
            }

            return logMessage;
        }

    private:
        const std::string COLOR_DEBUG = "\033[36m"; /**< DEBUG 颜色 */
        const std::string COLOR_INFO = "\033[32m";  /**< INFO 颜色 */
        const std::string COLOR_WARN = "\033[33m";  /**< WARN 颜色 */
        const std::string COLOR_ERROR = "\033[31m"; /**< ERROR 颜色 */
        const std::string COLOR_FATAL = "\033[35m"; /**< FATAL 颜色 */
        const std::string COLOR_RESET = "\033[0m";  /**< 颜色重置 */
        Color _enable_color;                        /**< 颜色启用状态 */
    };

    /**
     * @class FileSink
     * @brief 文件日志落地实现
     *
     * 该类实现了将日志写入到指定文件的功能。
     */
    class FileSink : public LogSink
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param pathname 文件路径
         *
         * 创建并打开指定的日志文件。
         */
        FileSink(const std::string &pathname)
            : _pathname(pathname)
        {
            Util::File::createDirectory(Util::File::path(_pathname)); // 创建目录
            _ofs.open(_pathname, std::ios::binary | std::ios::app);   // 打开文件
            assert(_ofs.is_open());
        }
        /**
         * @brief 日志写入到文件
         *
         * @param data 日志数据
         * @param len 数据长度
         *
         * 将日志数据写入文件。
         */
        void log(const char *data, size_t len)
        {
            _ofs.write(data, len);
            assert(_ofs.good());
        }

    private:
        std::string _pathname; /**< 文件路径 */
        std::ofstream _ofs;    /**< 文件输出流 */
    };
    /**
     * @class RollSinkBySize
     * @brief 基于文件大小的滚动文件日志落地实现
     *
     * 该类实现了将日志写入到文件，并在文件大小超过限制时创建新文件的功能。
     */
    class RollSinkBySize : public LogSink
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param basename 基础文件名
         * @param max_size 最大文件大小
         *
         * 创建并打开新的日志文件。
         */
        RollSinkBySize(const std::string &basename, size_t max_size)
            : _basename(basename), _max_fsize(max_size), _current_fsize(0), _cnt(0)
        {
            std::string pathname = creatNewFIle();
            Util::File::createDirectory(Util::File::path(pathname)); // 创建目录
            _ofs.open(pathname, std::ios::binary | std::ios::app);
            assert(_ofs.is_open());
        }
        /**
         * @brief 日志写入到滚动文件
         *
         * @param data 日志数据
         * @param len 数据长度
         *
         * 将日志数据写入文件，如果当前文件大小超过限制，则创建新文件。
         */
        void log(const char *data, size_t len)
        {
            if (_current_fsize >= _max_fsize)
            {
                std::string pathname = creatNewFIle();
                _ofs.close(); // 关闭原来已经打开的文件
                _ofs.open(pathname, std::ios::binary | std::ios::app);
                assert(_ofs.is_open());
                _current_fsize = 0;
                // _cnt = 0;
            }
            _ofs.write(data, len);
            assert(_ofs.good());
            _current_fsize += len;
        }

    private:
        /**
         * @brief 创建新文件
         *
         * @return std::string 新创建的文件路径
         *
         * 根据当前时间生成新文件名。
         */
        std::string creatNewFIle() // 大小判断，超过则创建新文件
        {
            // 获取系统时间，构造文件扩展名
            time_t t = Util::Date::getTime();
            struct tm lt;
            localtime_r(&t, &lt);
            std::stringstream filename;
            filename << _basename << lt.tm_year + 1900 << lt.tm_mon + 1 << lt.tm_mday << lt.tm_hour << lt.tm_min << lt.tm_sec << "-" << _cnt++ << ".log";
            return filename.str();
        }

    private:
        std::string _basename; /**< 基础文件名 */
        std::ofstream _ofs;    /**< 文件输出流 */
        size_t _max_fsize;     /**< 最大文件大小 */
        size_t _current_fsize; /**< 当前文件大小 */
        size_t _cnt;           /**< 文件计数 */
    };

    /**
     * @class SinkFactory
     * @brief 日志落地对象工厂类
     *
     * 该类实现了简单的工厂模式，用于创建不同类型的 LogSink 对象。
     */
    class SinkFactory
    {
    public:
        /**
         * @brief 创建日志落地对象
         *
         * @tparam SinkType 日志落地类型
         * @tparam Args 构造函数参数类型
         * @param args 构造函数参数
         * @return LogSink::ptr 创建的日志落地对象
         */
        template <typename SinkType, typename... Args>
        static LogSink::ptr create(Args &&...args)
        {
            return std::make_shared<SinkType>(std::forward<Args>(args)...);
        }
    };
}
