/// @file serverlog.hpp
/// @brief 服务器实际执行落地的类
#pragma once
#include "../extend/DataBaseSink.hpp"
#include "codec.hpp"
#include "server.hpp"
namespace XuServer
{
    /// @class ServerLog
    /// @brief 服务器实际执行落地的类
    class ServerLog
    {
    public:
        using ptr = std::shared_ptr<ServerLog>; ///< 服务器实际落地操作句柄
        
        /// @brief 获取服务器落地操作句柄
        /// @return 服务器落地操作句柄
        static ptr getInstance()
        {
            if (_log == nullptr)
                _log = std::make_shared<ServerLog>();
            return _log;
        }
        /// @brief 构造函数
        ServerLog()
        {
            if (_builder == nullptr)
            {
                // 日志器配置
                _builder = std::make_shared<Xulog::GlobalLoggerBuild>();
                _builder->buildLoggerName("server");
                _builder->buildFormatter();
                _builder->buildLoggerType();
                _builder->build();
            }
        }
        /// @brief 仿函数
        /// @param msg JsonData字符串
        void operator()(std::string &msg)
        {
            _logger = Xulog::getLogger("server");
            // std::cout << _sinks.size() << std::endl;
            Json::CharReaderBuilder reader;
            Json::Value root;
            std::istringstream(msg) >> root;
            Xulog::DeliverMsg dmsg = Xulog::Codec::fromJson(root);
            std::string str;
            if (dmsg.msg_mod == "format")
            {
                if (_logger == nullptr)
                    std::cout << "_logger 空指针！！" << std::endl;
                else
                {
                    if (_logger->getFormatter() == nullptr)
                        std::cout << "getFormatter 空指针！！" << std::endl;
                    else
                        str = _logger->getFormatter()->Format(dmsg.format_msg);
                }
            }
            else
                str = dmsg.unformatted_msg;
            std::vector<Xulog::LogSink::ptr> _sinks;
            init(_sinks);
            for (auto sink : _sinks)
            {
                DataBaseSink::DBptr dbptr = std::dynamic_pointer_cast<DataBaseSink>(sink);
                if (dbptr == nullptr)
                    sink->log(str.c_str(), str.size());
                else
                    dbptr->log(dmsg.format_msg);
            }
        }
        /// @brief 服务器收到消息的处理回调函数
        /// @param msg 服务器接收的信息
        /// @param error_code 错误码
        /// @return 需要返回给客户端的信息
        static std::string logMsg(std::vector<char> &msg, bool *error_code)
        {
            XuServer::ServerLog::ptr log = XuServer::ServerLog::getInstance();
            size_t offset = 0;
            while (offset < msg.size())
            {

                if (msg.size() < sizeof(uint32_t))
                {
                    break; // 检查消息大小
                }

                uint32_t sz_net;
                ::memcpy(&sz_net, msg.data() + offset, sizeof(uint32_t)); // 从正确的位置复制
                uint32_t sz = ntohl(sz_net);

                if (offset + sizeof(uint32_t) + sz > msg.size())
                {
                    break;
                }

                std::string jsonData(msg.begin() + offset + sizeof(uint32_t), msg.begin() + offset + sizeof(uint32_t) + sz);
                (*log)(jsonData);
                offset += (sizeof(uint32_t) + sz);
            }
            msg.clear();
            return std::string();
        }

    private:
        /// @brief 初始化落地方式
        /// @param sinks
        void init(std::vector<Xulog::LogSink::ptr> &sinks)
        {
            XuServer::Config::ptr config = XuServer::Config::getInstance();

            std::string cfg = config->get("StdoutSink", "color");
            if (cfg == "true")
                sinks.push_back(std::make_shared<Xulog::StdoutSink>(Xulog::StdoutSink::Color::Enable));
            else if (cfg == "false" || cfg == "default")
                sinks.push_back(std::make_shared<Xulog::StdoutSink>(Xulog::StdoutSink::Color::Unenable));

            cfg = config->get("FileSink", "path");
            if (cfg == "default")
                sinks.push_back(std::make_shared<Xulog::FileSink>("./log/test.log"));
            else if (!cfg.empty())
                sinks.push_back(std::make_shared<Xulog::FileSink>(cfg));

            cfg = config->get("RollBySize", "path");
            if (cfg == "default")
            {
                std::string sz = config->get("RollBySize", "size");
                if (sz.empty() || sz == "default")
                    sz = "1024";
                int size = stoi(sz) * 1024;
                sinks.push_back(std::make_shared<Xulog::RollSinkBySize>("./log/roll-", size));
            }
            else if (!cfg.empty())
            {
                std::string sz = config->get("RollBySize", "size");
                if (sz.empty() || sz == "default")
                    sz = "1024";
                int size = stoi(sz) * 1024;
                sinks.push_back(std::make_shared<Xulog::RollSinkBySize>(cfg, size));
            }

            cfg = config->get("RollByTime", "path");
            if (cfg == "default")
            {
                std::string tp = config->get("RollByTime", "type");
                if (tp == "GAP_MINUTE")
                    sinks.push_back(std::make_shared<RollSinkByTime>("./log/roll-", TimeGap::GAP_MINUTE));
                else if (tp == "GAP_HOUR")
                    sinks.push_back(std::make_shared<RollSinkByTime>("./log/roll-", TimeGap::GAP_HOUR));
                else if (tp == "GAP_DAY")
                    sinks.push_back(std::make_shared<RollSinkByTime>("./log/roll-", TimeGap::GAP_DAY));
                else
                    sinks.push_back(std::make_shared<RollSinkByTime>("./log/roll-", TimeGap::GAP_SECOND));
            }
            else if (!cfg.empty())
            {
                std::string tp = config->get("RollByTime", "type");
                if (tp == "GAP_MINUTE")
                    sinks.push_back(std::make_shared<RollSinkByTime>(cfg, TimeGap::GAP_MINUTE));
                else if (tp == "GAP_HOUR")
                    sinks.push_back(std::make_shared<RollSinkByTime>(cfg, TimeGap::GAP_HOUR));
                else if (tp == "GAP_DAY")
                    sinks.push_back(std::make_shared<RollSinkByTime>(cfg, TimeGap::GAP_DAY));
                else
                    sinks.push_back(std::make_shared<RollSinkByTime>(cfg, TimeGap::GAP_SECOND));
            }
            cfg = config->get("DataBaseSink", "path");
            if (cfg == "default")
                sinks.push_back(std::make_shared<DataBaseSink>("./log/log.db", "server"));
            else if (!cfg.empty())
                sinks.push_back(std::make_shared<DataBaseSink>(cfg, "server"));
        }

    private:
        static std::shared_ptr<Xulog::LoggerBuilder> _builder; ///< 日志构造器
        static ptr _log;                                       ///< 服务器实际落地操作句柄

        Xulog::Logger::ptr _logger; ///< 日志器句柄
    };
    ServerLog::ptr ServerLog::_log = nullptr;                            ///< 服务器实际落地操作句柄
    std::shared_ptr<Xulog::LoggerBuilder> ServerLog::_builder = nullptr; ///< 日志器初始化
}