/// @file codec.hpp
/// @brief 对日志信息进行序列化和反序列化
#pragma once
#include <jsoncpp/json/json.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <cstring>
#include <jsoncpp/json/json.h>
#include "../logs/message.hpp"

namespace Xulog
{
    /// @struct DeliverMsg
    /// @brief 传递的消息
    struct DeliverMsg
    {
        std::string msg_mod;         ///< 消息模式
        std::string unformatted_msg; ///< 非结构化消息
        LogMsg format_msg;           ///< 结构化信息
    };
    /// @classCodec
    /// @brief 序列化与反序列化类
    class Codec
    {
    public:
        /// @brief 将结构化信息序列化为Json格式
        /// @param msg 结构化信息
        /// @return Json格式
        static Json::Value toJson(const LogMsg &msg)
        {
            Json::Value json;
            json["format_msg"] = msgToJson(msg);
            json["msg_mod"] = "format";
            return json;
        }
        /// @brief 将非结构化信息序列化为Json格式
        /// @param msg 非结构化信息
        /// @return Json格式
        static Json::Value toJson(const std::string &msg)
        {
            Json::Value json;
            json["unformatted_msg"] = msg;
            json["msg_mod"] = "unformatted";
            return json;
        }
        /// @brief 将Json信息反序列化为传递消息
        /// @param json Json信息
        /// @return 传递消息
        static DeliverMsg fromJson(const Json::Value &json)
        {
            DeliverMsg dmsg;
            dmsg.msg_mod = json["msg_mod"].asString();
            if (dmsg.msg_mod == "format")
            {
                dmsg.format_msg = msgFromJson(json["format_msg"]);
            }
            else
                dmsg.unformatted_msg = json["unformatted_msg"].asString();
            return dmsg;
        }

    private:
        /// @brief 将结构化信息序列化为Json格式
        /// @param msg 结构化信息
        /// @return Json格式
        static Json::Value msgToJson(const LogMsg &msg)
        {
            Json::Value json;
            json["ctime"] = static_cast<Json::Int64>(msg._ctime);
            json["line"] = msg._line;
            json["tid"] = std::to_string(*(unsigned long int *)(&msg._tid));
            json["level"] = LogLevel::toString(msg._level);
            json["file"] = msg._file;
            json["logger"] = msg._logger;
            json["payload"] = msg._payload;
            return json;
        }
        /// @brief 将Json信息反序列化为传递消息
        /// @param json Json信息
        /// @return 传递消息
        static LogMsg msgFromJson(const Json::Value &json)
        {
            LogMsg msg;
            msg._ctime = json["ctime"].asInt64();
            msg._line = json["line"].asUInt();
            msg._tid = std::thread::id(std::stoul(json["tid"].asString()));
            msg._level = LogLevel::fromString(json["level"].asString());
            msg._file = json["file"].asString();
            msg._logger = json["logger"].asString();
            msg._payload = json["payload"].asString();
            return msg;
        }
    };
}