/**
 * @file format.hpp
 * @brief 日志格式化器类的定义
 *
 * 本文件定义了日志格式化器类及其相关格式化子项。日志格式化器用于将日志消息格式化为字符串，并输出到指定流。
 * 支持的格式化子项包括时间、日志级别、文件名、行号、线程ID等。用户可以自定义日志的输出格式。
 */
#pragma once

#include "level.hpp"
#include "message.hpp"
#include <ctime>
#include <vector>
#include <cassert>
#include <sstream>
#include <memory>
namespace Xulog
{
    /**
     * @brief 抽象格式化子项的基类
     */
    class FormatItem
    {
    public:
        using ptr = std::shared_ptr<FormatItem>;
        /**
         * @brief 格式化日志消息
         * @param out 输出流
         * @param msg 日志消息
         */
        virtual void format(std::ostream &out, LogMsg &msg) = 0;
    };
    /**
     * @brief 消息格式化子项
     */
    class MsgFormatItem : public FormatItem
    {
    public:
        /**
         * @brief 格式化消息
         * @param out 输出流
         * @param msg 日志消息
         */
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << msg._payload;
        }
    };
    /**
     * @brief 日志级别格式化子项
     */
    class LevelFormatItem : public FormatItem
    {
    public:
        /**
         * @brief 格式化日志级别
         * @param out 输出流
         * @param msg 日志消息
         */
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << LogLevel::toString(msg._level);
        }
    };
    /**
     * @brief 时间格式化子项
     */
    class TimeFormatItem : public FormatItem
    {
    public:
        /**
         * @brief 构造时间格式化子项
         * @param fmt 时间格式，默认为"%H:%M:%S"
         */
        TimeFormatItem(const std::string &fmt = "%H:%M:%S")
            : _time_fmt(fmt)
        {
        }
        /**
         * @brief 格式化时间
         * @param out 输出流
         * @param msg 日志消息
         */
        void format(std::ostream &out, LogMsg &msg) override
        {
            struct tm st;
            localtime_r(&msg._ctime, &st);
            char tmp[32] = {0};
            strftime(tmp, 31, _time_fmt.c_str(), &st);
            out << tmp;
        }

    private:
        std::string _time_fmt; ///< 时间格式
    };
    /**
     * @brief 文件名格式化子项
     */
    class FileFormatItem : public FormatItem
    {
    public:
        /**
         * @brief 格式化文件名
         * @param out 输出流
         * @param msg 日志消息
         */
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << msg._file;
        }
    };
    /**
     * @brief 行号格式化子项
     */
    class LineFormatItem : public FormatItem
    {
    public:
        /**
         * @brief 格式化行号
         * @param out 输出流
         * @param msg 日志消息
         */
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << msg._line;
        }
    };
    /**
     * @brief 线程ID格式化子项
     */
    class ThreadFormatItem : public FormatItem
    {
    public:
        /**
         * @brief 格式化线程ID
         * @param out 输出流
         * @param msg 日志消息
         */
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << msg._tid;
        }
    };
    /**
     * @brief 日志器名称格式化子项
     */
    class LoggerFormatItem : public FormatItem
    {
    public:
        /**
         * @brief 格式化日志器名称
         * @param out 输出流
         * @param msg 日志消息
         */
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << msg._logger;
        }
    };
    /**
     * @brief 制表符格式化子项
     */
    class TabFormatItem : public FormatItem
    {
    public:
        /**
         * @brief 格式化制表符
         * @param out 输出流
         * @param msg 日志消息
         */
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << "\t";
        }
    };
    /**
     * @brief 换行符格式化子项
     */
    class NLineFormatItem : public FormatItem
    {
    public:
        /**
         * @brief 格式化换行符
         * @param out 输出流
         * @param msg 日志消息
         */
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << "\n";
        }
    };
    /**
     * @brief 其他格式化子项
     */
    class OtherFormatItem : public FormatItem
    {
    public:
        /**
         * @brief 构造其他格式化子项
         * @param str 格式化字符串
         */
        OtherFormatItem(const std::string &str)
            : _str(str)
        {
        }
        /**
         * @brief 格式化其他字符串
         * @param out 输出流
         * @param msg 日志消息
         */
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << _str;
        }

    private:
        std::string _str; ///< 格式化字符串
    };

    /*
        %d 日期，子格式{%H:%M:%S}
        %T 缩进
        %t 线程ID
        %p 日志级别
        %c 日志器名称
        %f 文件名
        %l 行号
        %m 日志消息
        %n 换行
    */

    /**
     * @brief 格式化器类，负责将日志消息格式化为字符串
     */
    class Formatter
    {
    public:
        using ptr = std::shared_ptr<Formatter>;
        /**
         * @brief 构造格式化器
         * @param pattern 格式化规则字符串，默认为"[%d{%y-%m-%d|%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n"
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
        Formatter(const std::string &pattern = "[%d{%y-%m-%d|%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n")
            : _pattern(pattern)
        {
            assert(parsePattern());
        }
        /**
         * @brief 对日志消息进行格式化
         * @param msg 日志消息
         * @return 格式化后的字符串
         */
        std::string Format(LogMsg &msg)
        {
            std::stringstream ss;
            Format(ss, msg);
            return ss.str();
        }
        /**
         * @brief 对日志消息进行格式化，并输出到指定流
         * @param out 输出流
         * @param msg 日志消息
         */
        void Format(std::ostream &out, LogMsg &msg)
        {
            for (auto &item : _items)
            {
                item->format(out, msg);
            }
        }
        // 对格式化内容进行解析
        /*
            1. 没有以%起始的字符都是原始字符串
            2. 遇到%则是原始字符串的结束
            3. %%表示原始的%
            4. 如果遇到{则说明这是格式化字符的子格式，遇到}结束
        */
       
        /// @brief 获取格式化字符串
        /// @return 格式化字符串
        std::string getPattern()
        {
            return _pattern;
        }

    private:
        /**
         * @brief 解析格式化规则字符串
         * @return 是否成功解析
         */
        bool parsePattern()
        {
            std::vector<std::pair<std::string, std::string>> fmt_order;
            size_t pos = 0;
            std::string key, val;

            // 字符串解析
            while (pos < _pattern.size())
            {
                if (_pattern[pos] != '%')
                {
                    val.push_back(_pattern[pos++]);
                    continue;
                }
                if (pos + 1 < _pattern.size() && _pattern[pos + 1] == '%')
                {
                    val.push_back('%');
                    pos += 2;
                    continue;
                }
                // 原始字符串处理完毕
                if (val.empty() == false)
                {
                    fmt_order.push_back(std::make_pair("", val));
                    val.clear();
                }

                if (pos >= _pattern.size())
                {
                    std::cout << "%后没有有效字符" << std::endl;
                    return false;
                }
                pos += 1;
                key = _pattern[pos];
                if (pos + 1 < _pattern.size() && _pattern[pos + 1] == '{')
                {
                    pos += 2;
                    while (pos < _pattern.size() && _pattern[pos] != '}')
                    {
                        val.push_back(_pattern[pos++]);
                    }
                    if (pos >= _pattern.size())
                    {
                        std::cout << "规则中{}匹配出错" << std::endl;
                        return false;
                    }
                }

                fmt_order.push_back(std::make_pair(key, val));
                pos++;
                key.clear();
                val.clear();
            }

            // 初始化成员
            for (auto &it : fmt_order)
            {
                _items.push_back(createItem(it.first, it.second));
            }
            return true;
        }
        /**
         * @brief 根据格式化字符创建格式化子项对象
         * @param key 格式化字符
         * @param value 子格式
         * @return 格式化子项的指针
         */
        FormatItem::ptr createItem(const std::string &key, const std::string &value)
        {
            if (key == "d")
                return std::make_shared<TimeFormatItem>(value);
            if (key == "t")
                return std::make_shared<ThreadFormatItem>();
            if (key == "c")
                return std::make_shared<LoggerFormatItem>();
            if (key == "f")
                return std::make_shared<FileFormatItem>();
            if (key == "l")
                return std::make_shared<LineFormatItem>();
            if (key == "p")
                return std::make_shared<LevelFormatItem>();
            if (key == "T")
                return std::make_shared<TabFormatItem>();
            if (key == "m")
                return std::make_shared<MsgFormatItem>();
            if (key == "n")
                return std::make_shared<NLineFormatItem>();
            if (key.empty())
                return std::make_shared<OtherFormatItem>(value);
            std::cout << "没有对应的格式化字符:'%" << key << "'\n";
            exit(1);
            return FormatItem::ptr();
        }

    private:
        std::string _pattern;                ///< 格式化规则字符串
        std::vector<FormatItem::ptr> _items; ///< 格式化子项集合
    };
}
