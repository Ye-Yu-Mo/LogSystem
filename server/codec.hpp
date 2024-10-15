/// @file codec.hpp
/// @brief 对日志信息进行序列化和反序列化
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
    struct DeliverMsg
    {
        LogMsg msg;
        std::string logger_name;
        Xulog::LogLevel::value limit_level;
        std::string pattern;
    };
    class Codec
    {
    public:
        static Json::Value toJson(const LogMsg &msg, Xulog::Logger::ptr logger)
        {
            Json::Value json;
            json["msg"] = msgToJson(msg);
            json["logger_name"] = logger->getName();
            json["limit_level"] = Xulog::LogLevel::toString(logger->getLimitLevel());
            json["pattern"] = logger->getFormatter()->getPattern();
            return json;
        }
        static DeliverMsg fromJson(const Json::Value &json)
        {
            DeliverMsg dmsg;
            dmsg.msg = msgFromJson(json["msg"]);
            dmsg.logger_name = json["logger_name"].asString();
            dmsg.limit_level = Xulog::LogLevel::fromString(json["limit_level"].asString());
            dmsg.pattern = json["pattern"].asString();
            return dmsg;
        }
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