/*
    日志消息类的实现，存储中间日志信息
        1. 日志输出时间
        2. 日志等级
        3. 源文件名称
        4. 源代码行号
        5. 线程ID
        6. 实际的日志主体消息
        7. 日志器名称
*/
#pragma once

#include <iostream>
#include <thread>
#include <string>
#include "level.hpp"
#include "util.hpp"

namespace Xulog
{
    struct LogMsg
    {
        time_t _ctime;          // 日志产生的时间戳
        size_t _line;           // 行号
        std::thread::id _tid;   // 线程ID
        LogLevel::value _level; // 日志等级
        std::string _file;      // 源文件名称
        std::string _logger;    // 日志器
        std::string _payload;   // 有效载荷数据

        LogMsg(LogLevel::value level,
               size_t line,
               const std::string file,
               const std::string logger,
               const std::string msg) : _ctime(Util::Date::getTime()),
                                        _level(level),
                                        _line(line),
                                        _tid(std::this_thread::get_id()),
                                        _file(file),
                                        _logger(logger),
                                        _payload(msg)
        {
        }
    };
}
