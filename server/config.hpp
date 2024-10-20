/// @file config.hpp
/// @brief 用于读取和存储配置文件的类
#pragma once
#include "../config/INIReader.h"
#include "../logs/Xulog.h"
#include <unordered_map>
#include <string>
namespace XuServer
{
    /// @class Config
    /// @brief 用于读取和存储配置文件的类
    class Config
    {
    public:
        /// @brief 构造函数
        /// @param filename 配置文件路径
        Config(const std::string &filename)
        {
            INIReader reader(filename);
            if (reader.ParseError() < 0)
            {
                ERROR("配置解析失败！");
            }
            init(reader);
        }
        using ptr = std::shared_ptr<Config>;                                                              ///< 配置管理句柄
        using config_map = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>; ///< 从section和name获取value的映射表

        /// @brief 获取配置文件操作句柄
        /// @param filename 配置文件路径
        /// @return 配置文件操作句柄
        static Config::ptr getInstance(const std::string &filename)
        {
            if (_instance == nullptr)
                _instance = std::make_shared<Config>(filename);
            return _instance;
        }
        /// @brief 获取配置文件操作句柄
        /// @return 配置文件操作句柄
        static Config::ptr getInstance()
        {
            if (_instance == nullptr)
                ERROR(" 请传入配置文件路径！");
            return _instance;
        }
        /// @brief 获取配置文件的值
        /// @param section section 方括号中的值
        /// @param name 字段名称
        /// @return 字段值
        std::string get(const std::string &section, const std::string &name)
        {
            auto sec_it = _config_data.find(section);
            if (sec_it != _config_data.end())
            {
                auto name_it = sec_it->second.find(name);
                if (name_it != sec_it->second.end())
                {
                    return name_it->second;
                }
            }
            return "";
        }

    private:
        /// @brief 读取配置文件并写入到映射表中
        /// @param reader ini读取器
        void init(INIReader &reader)
        {
            std::string ret = reader.Get("Server", "port", "8888");
            _config_data["Server"]["port"] = ret;
            ret = reader.Get("StdoutSink", "color", "");
            if (!ret.empty())
            {
                _config_data["StdoutSink"]["color"] = ret;
            }

            ret = reader.Get("FileSink", "path", "");
            if (!ret.empty())
            {
                _config_data["FileSink"]["path"] = ret;
            }

            ret = reader.Get("RollBySize", "path", "");
            if (!ret.empty())
            {
                std::string r = reader.Get("RollBySize", "size", "1024");
                _config_data["RollBySize"]["path"] = ret;
                _config_data["RollBySize"]["size"] = r;
            }

            ret = reader.Get("RollByTime", "path", "");
            if (!ret.empty())
            {
                std::string r = reader.Get("RollByTime", "type", "GAP_SECOND");
                _config_data["RollByTime"]["path"] = ret;
                _config_data["RollByTime"]["type"] = r;
            }

            ret = reader.Get("DataBaseSink", "path", "");
            if (!ret.empty())
            {
                _config_data["DataBaseSink"]["path"] = ret;
            }
        }

    private:
        config_map _config_data; ///< 从section和name获取value的映射表
        static ptr _instance;    ///< 配置文件操作句柄
    };
    Config::ptr Config::_instance = nullptr; ///< 配置文件操作句柄
}