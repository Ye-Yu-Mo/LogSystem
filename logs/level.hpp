/**
 * @file level.hpp
 * @brief 日志等级类的定义和实现
 *
 * 该文件包含了日志等级的枚举定义，并提供了将日志等级转换为字符串的接口。
 */
#pragma once

namespace Xulog
{
    /**
     * @class LogLevel
     * @brief 日志等级类
     *
     * 该类定义了不同的日志等级，并提供了日志等级与字符串之间的转换方法。
     */
    class LogLevel
    {
    public:
        /**
         * @enum value
         * @brief 日志等级的枚举值
         *
         * 定义日志的不同等级，从UNKNOW到OFF。
         */
        enum class value
        {
            UNKNOW = 0, /**< 未知日志等级 */
            DEBUG,      /**< 调试信息 */
            INFO,       /**< 普通信息 */
            WARN,       /**< 警告信息 */
            ERROR,      /**< 错误信息 */
            FATAL,      /**< 严重错误 */
            OFF         /**< 日志关闭 */
        };
        /**
         * @brief 将日志等级转换为对应的字符串
         *
         * @param level 日志等级的枚举值
         * @return 对应的日志等级字符串
         *
         * 该函数根据传入的日志等级枚举值，返回对应的字符串表示。
         */
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
            default:
                return "UNKNOW";
            }
        }
        /// @brief 从字符串转换成日志等级
        /// @param level 日志等级字符串
        /// @return 日志等级
        static LogLevel::value fromString(const std::string& level)
        {
            if(level=="DEBUG")
                return LogLevel::value::DEBUG;
            else if(level=="INFO")
                return LogLevel::value::INFO;
            else if(level=="WARN")
                return LogLevel::value::WARN;
            else if(level=="ERROR")
                return LogLevel::value::ERROR;
            else if(level=="FATAL")
                return LogLevel::value::FATAL;
            else if(level=="OFF")
                return LogLevel::value::OFF;
            else
                return LogLevel::value::UNKNOW;
        }
    };
}