#pragma once
#include "logger.hpp"

namespace Xulog
{
    // 提供获取制定日志器的全局接口，避免用户直接调用单例对象
    Logger::ptr getLogger(const std::string &name)
    {
        return LoggerManager::getInstance().getLogger(name);
    }
    Logger::ptr rootLogger()
    {
        return LoggerManager::getInstance().rootLogger();
    }

// 使用宏函数对日志器进行代理
#define debug(logger, fmt, ...) logger->debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define error(logger, fmt, ...) logger->error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define warn(logger, fmt, ...) logger->warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define info(logger, fmt, ...) logger->info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define fatal(logger, fmt, ...) logger->fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
// 提供宏函数，直接通过默认日志器进行日志标准输出打印
#define DEBUG(fmt, ...) debug(Xulog::rootLogger(), fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) error(Xulog::rootLogger(), fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) warn(Xulog::rootLogger(), fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) info(Xulog::rootLogger(), fmt, ##__VA_ARGS__)
#define FATAL(fmt, ...) fatal(Xulog::rootLogger(), fmt, ##__VA_ARGS__)

}
