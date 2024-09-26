/*
    日志落地模块的实现
        1. 抽象落地基类
        2. 派生子类
        3. 使用工厂模式进行创建与表示的分离
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
    class LogSink
    {
    public:
        using ptr = std::shared_ptr<LogSink>;
        LogSink() {}
        virtual ~LogSink() {}
        virtual void log(const char *data, size_t len) = 0;
    };
    // 标准输出
    class StdoutSink : public LogSink
    {
    public:
        enum class Color
        {
            Enable,
            Unenable
        };
        StdoutSink(Color enable = Color::Unenable)
            : _enable_color(enable)
        {
        }
        // 日志写入到标准输出
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
        const std::string COLOR_DEBUG = "\033[36m";
        const std::string COLOR_INFO = "\033[32m";
        const std::string COLOR_WARN = "\033[33m";
        const std::string COLOR_ERROR = "\033[31m";
        const std::string COLOR_FATAL = "\033[35m";
        const std::string COLOR_RESET = "\033[0m";
        Color _enable_color;
    };

    // 指定文件
    class FileSink : public LogSink
    {
    public:
        // 传入文件名时，构造并打开文件，将操作句柄管理起来
        FileSink(const std::string &pathname)
            : _pathname(pathname)
        {
            Util::File::createDirectory(Util::File::path(_pathname)); // 创建目录
            _ofs.open(_pathname, std::ios::binary | std::ios::app);   // 打开文件
            assert(_ofs.is_open());
        }
        void log(const char *data, size_t len)
        {
            _ofs.write(data, len);
            assert(_ofs.good());
        }

    private:
        std::string _pathname;
        std::ofstream _ofs;
    };
    // 滚动文件（大小）
    class RollSinkBySize : public LogSink
    {
    public:
        RollSinkBySize(const std::string &basename, size_t max_size)
            : _basename(basename), _max_fsize(max_size), _current_fsize(0), _cnt(0)
        {
            std::string pathname = creatNewFIle();
            Util::File::createDirectory(Util::File::path(pathname)); // 创建目录
            _ofs.open(pathname, std::ios::binary | std::ios::app);
            assert(_ofs.is_open());
        }
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
        std::string _basename; // 基础文件名 (+扩展文件名-时间|计数器)
        std::ofstream _ofs;
        size_t _max_fsize;     // 大小上限
        size_t _current_fsize; // 当前大小
        size_t _cnt;           // 日志数量
    };

    // 简单工厂模式
    class SinkFactory
    {
    public:
        template <typename SinkType, typename... Args>
        static LogSink::ptr create(Args &&...args)
        {
            return std::make_shared<SinkType>(std::forward<Args>(args)...);
        }
    };
}
