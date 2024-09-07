/*
    实用工具类的实现
        1. 获取系统时间
        2. 判断文件是否存在
        3，获取文件路径
        4. 创建目录
*/
#pragma once

#include <iostream>
#include <ctime>
#include <sys/stat.h>

namespace Xulog
{
    namespace Util
    {
        class Date
        {
        public:
            static size_t getTime()
            {
                return (size_t)time(nullptr); // 获取当前时间戳
            }
        };
        class File
        {
        public:
            static bool exists(const std::string &pathname)
            {
                struct stat st;
                if (stat(pathname.c_str(), &st) < 0) // 判断文件是否存在
                    return false;
                return true;
            }
            static std::string path(const std::string &pathname)
            {
                size_t pos = pathname.find_last_of("/\\"); // 查找最后一个'/'或者'\'
                if (pos == std::string::npos)
                    return ".";                     // 如果没有找到路径分隔符，返回当前目录
                return pathname.substr(0, pos + 1); // 获取文件路径的目录部分
            }
            static void createDirectory(const std::string &pathname)
            {
                size_t pos = 0, idx = 0; // pos表示'/'的位置，idx表示起始位置
                while (idx < pathname.size())
                {
                    pos = pathname.find_first_of("/\\", idx); // 找第一个'\'
                    if (pos == std::string::npos)             // 如果没有任何目录，直接创建
                    {
                        mkdir(pathname.c_str(), 0777);
                        return;
                    }
                    std::string parent_dir = pathname.substr(0, pos + 1); // 找到父级目录
                    if (exists(parent_dir) == true)                       // 如果存在则直接找下一个
                    {
                        idx = pos + 1;
                        continue;
                    }
                    mkdir(parent_dir.c_str(), 0777);
                    idx = pos + 1;
                }
            }
        };
    }
}