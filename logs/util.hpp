/**
 * @file util.hpp
 * @brief 实用工具类的实现
 *
 * 本文件提供了一些实用工具类，包括获取系统时间、判断文件是否存在、获取文件路径和创建目录的功能。
 */
#pragma once

#include <iostream>
#include <ctime>
#include <sys/stat.h>
#include <fstream>

namespace Xulog
{
    namespace Util
    {
        /**
         * @class Date
         * @brief 时间相关的实用工具类
         *
         * 提供获取当前系统时间的功能。
         */
        class Date
        {
            /**
             * @brief 获取当前时间戳
             *
             * @return size_t 当前时间戳（自1970年1月1日以来的秒数）
             */
        public:
            static size_t getTime()
            {
                return (size_t)time(nullptr); // 获取当前时间戳
            }
        };
        /**
         * @class File
         * @brief 文件操作相关的实用工具类
         *
         * 提供判断文件是否存在、获取文件路径和创建目录的功能。
         */
        class File
        {
        public:
            /**
             * @brief 判断文件是否存在
             *
             * @param pathname 文件的路径
             * @return true 如果文件存在
             * @return false 如果文件不存在
             */
            static bool exists(const std::string &pathname)
            {
                struct stat st;
                if (stat(pathname.c_str(), &st) < 0) // 判断文件是否存在
                    return false;
                return true;
            }
            /**
             * @brief 获取文件路径
             *
             * @param pathname 文件的路径
             * @return std::string 文件所在的目录路径
             *
             * 如果没有找到路径分隔符，返回当前目录。
             */
            static std::string path(const std::string &pathname)
            {
                size_t pos = pathname.find_last_of("/\\"); // 查找最后一个'/'或者'\'
                if (pos == std::string::npos)
                    return ".";                     // 如果没有找到路径分隔符，返回当前目录
                return pathname.substr(0, pos + 1); // 获取文件路径的目录部分
            }
            /**
             * @brief 创建目录及其父级目录
             *
             * @param pathname 要创建的目录的路径
             *
             * 逐层创建目录，直到完整路径创建完成。
             */
            static void createDirectory(const std::string &pathname)
            {
                size_t pos = 0, idx = 0; // pos表示'/'的位置，idx表示起始位置
                while (idx < pathname.size())
                {
                    pos = pathname.find_first_of("/\\", idx); // 找第一个'/'
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
            /**
             * @brief 创建新文件
             * @param filename 新文件名
             * @return true 创建成功，false 创建失败
             *
             * 在指定路径下创建一个新文件。
             */
            static bool createFile(const std::string filename)
            {
                std::fstream ofs(filename, std::ios::binary | std::ios::out);
                if (ofs.is_open() == false)
                {
                    return false;
                }
                ofs.close();
                return true;
            }
        };
    }
}