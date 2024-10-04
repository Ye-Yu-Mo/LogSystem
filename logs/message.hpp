/**
 * @file message.hpp
 * @brief 日志消息类的实现
 *
 * 本文件定义了日志消息类，用于存储日志输出的中间信息，包括时间、日志等级、源文件名称等。
 */
#pragma once

#include <iostream>
#include <thread>
#include <string>
#include "level.hpp"
#include "util.hpp"

namespace Xulog
{
    /**
     * @struct LogMsg
     * @brief 日志消息结构体
     *
     * 该结构体用于存储日志输出的相关信息，包括时间、日志等级、源文件名称、行号、线程ID和日志内容。
     */
    struct LogMsg
    {
        time_t _ctime;          ///< 日志产生的时间戳
        size_t _line;           ///< 行号
        std::thread::id _tid;   ///< 线程ID
        LogLevel::value _level; ///< 日志等级
        std::string _file;      ///< 源文件名称
        std::string _logger;    ///< 日志器
        std::string _payload;   ///< 有效载荷数据

        /**
         * @brief LogMsg 构造函数
         *
         * @param level 日志等级
         * @param line 源代码行号
         * @param file 源文件名称
         * @param logger 日志器名称
         * @param msg 日志主体消息
         *
         * 构造日志消息对象，并初始化所有相关字段。
         */
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
