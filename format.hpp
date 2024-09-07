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
    // 抽象格式化子项的基类
    class FormatItem
    {
    public:
        using ptr = std::shared_ptr<FormatItem>;
        virtual void format(std::ostream &out, LogMsg &msg) = 0;
    };
    // 派生格式化子项的子类
    class MsgFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << msg._payload;
        }
    };

    class LevelFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << LogLevel::toString(msg._level);
        }
    };

    class TimeFormatItem : public FormatItem
    {
    public:
        TimeFormatItem(const std::string &fmt = "%H:%M:%S")
            : _time_fmt(fmt)
        {
        }
        void format(std::ostream &out, LogMsg &msg) override
        {
            struct tm st;
            localtime_r(&msg._ctime, &st);
            char tmp[32] = {0};
            strftime(tmp, 31, _time_fmt.c_str(), &st);
            out << tmp;
        }

    private:
        std::string _time_fmt; // %H:%M:%S
    };

    class FileFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << msg._file;
        }
    };

    class LineFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << msg._line;
        }
    };

    class ThreadFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << msg._tid;
        }
    };

    class LoggerFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << msg._logger;
        }
    };

    class TabFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << "\t";
        }
    };

    class NLineFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << "\n";
        }
    };

    class OtherFormatItem : public FormatItem
    {
    public:
        OtherFormatItem(const std::string &str)
            : _str(str)
        {
        }
        void format(std::ostream &out, LogMsg &msg) override
        {
            out << _str;
        }

    private:
        std::string _str;
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

    class Formatter
    {
    public:
        Formatter(const std::string &pattern = "[%d{%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n")
            : _pattern(pattern)
        {
            assert(parsePattern());
        }
        // 对msg进行格式化
        std::string Format(LogMsg &msg)
        {
            std::stringstream ss;
            Format(ss, msg);
            return ss.str();
        }

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
    private:
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
                pos+=1;
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
        // 根据不同的格式化字符，创建不同的格式化子项的对象
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
            if(key.empty())
                return std::make_shared<OtherFormatItem>(value);
            std::cout<<"没有对应的格式化字符:'%"<<key<<"'\n";
            exit(1);
            return FormatItem::ptr();
        }

    private:
        std::string _pattern; // 格式化规则字符串
        std::vector<FormatItem::ptr> _items;
    };
}
