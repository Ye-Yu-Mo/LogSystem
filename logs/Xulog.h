/**
 * @file Xulog.h
 * @brief 全局日志接口和宏定义
 *
 * 该文件提供了全局获取日志器的接口以及方便使用日志器的宏定义，避免直接调用单例对象。
 */

#pragma once
#include "logger.hpp"

namespace Xulog
{
    /**
     * @brief 获取指定名称的日志器
     *
     * 通过名称获取对应的日志器，避免用户直接调用单例对象。
     *
     * @param name 日志器的名称
     * @return Logger::ptr 指向日志器的智能指针
     */
    Logger::ptr getLogger(const std::string &name)
    {
        return LoggerManager::getInstance().getLogger(name);
    }

    /**
     * @brief 获取默认日志器
     *
     * 提供全局接口获取默认日志器。
     *
     * @return Logger::ptr 指向默认日志器的智能指针
     */
    Logger::ptr rootLogger()
    {
        return LoggerManager::getInstance().rootLogger();
    }

/**
 * @def debug(logger, fmt, ...)
 * @brief 使用指定日志器打印调试信息
 *
 * @param logger 日志器对象
 * @param fmt 格式化的日志内容
 * @param ... 可变参数
 */
#define debug(logger, fmt, ...) logger->debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @def error(logger, fmt, ...)
 * @brief 使用指定日志器打印错误信息
 *
 * @param logger 日志器对象
 * @param fmt 格式化的日志内容
 * @param ... 可变参数
 */
#define error(logger, fmt, ...) logger->error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @def warn(logger, fmt, ...)
 * @brief 使用指定日志器打印警告信息
 *
 * @param logger 日志器对象
 * @param fmt 格式化的日志内容
 * @param ... 可变参数
 */
#define warn(logger, fmt, ...) logger->warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @def info(logger, fmt, ...)
 * @brief 使用指定日志器打印普通信息
 *
 * @param logger 日志器对象
 * @param fmt 格式化的日志内容
 * @param ... 可变参数
 */
#define info(logger, fmt, ...) logger->info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @def fatal(logger, fmt, ...)
 * @brief 使用指定日志器打印严重错误信息
 *
 * @param logger 日志器对象
 * @param fmt 格式化的日志内容
 * @param ... 可变参数
 */
#define fatal(logger, fmt, ...) logger->fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @def DEBUG(fmt, ...)
 * @brief 使用默认日志器打印调试信息
 *
 * @param fmt 格式化的日志内容
 * @param ... 可变参数
 */
#define DEBUG(fmt, ...) debug(Xulog::rootLogger(), fmt, ##__VA_ARGS__)

/**
 * @def ERROR(fmt, ...)
 * @brief 使用默认日志器打印错误信息
 *
 * @param fmt 格式化的日志内容
 * @param ... 可变参数
 */
#define ERROR(fmt, ...) error(Xulog::rootLogger(), fmt, ##__VA_ARGS__)

/**
 * @def WARN(fmt, ...)
 * @brief 使用默认日志器打印警告信息
 *
 * @param fmt 格式化的日志内容
 * @param ... 可变参数
 */
#define WARN(fmt, ...) warn(Xulog::rootLogger(), fmt, ##__VA_ARGS__)

/**
 * @def INFO(fmt, ...)
 * @brief 使用默认日志器打印普通信息
 *
 * @param fmt 格式化的日志内容
 * @param ... 可变参数
 */
#define INFO(fmt, ...) info(Xulog::rootLogger(), fmt, ##__VA_ARGS__)

/**
 * @def FATAL(fmt, ...)
 * @brief 使用默认日志器打印严重错误信息
 *
 * @param fmt 格式化的日志内容
 * @param ... 可变参数
 */
#define FATAL(fmt, ...) fatal(Xulog::rootLogger(), fmt, ##__VA_ARGS__)

}
