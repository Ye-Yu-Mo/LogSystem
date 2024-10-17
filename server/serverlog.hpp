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
            std::vector<Xulog::LogSink::ptr> sinks;
            init(sinks);
            for (auto sink : sinks)
                sink->log(str.c_str(), str.size());
        }

    private:
        /// @brief 初始化落地方式
        /// @param sinks
        void init(std::vector<Xulog::LogSink::ptr> &sinks)
        {
            Xulog::LogSink::ptr std = std::make_shared<Xulog::StdoutSink>();
            Xulog::FileSink::ptr file = std::make_shared<Xulog::FileSink>("./test.log");
            sinks.push_back(std);
            sinks.push_back(file);
        }

    private:
        static std::shared_ptr<Xulog::LoggerBuilder> _builder; ///< 日志构造器
        Xulog::Logger::ptr _logger;                            ///< 日志器句柄
    };
    std::shared_ptr<Xulog::LoggerBuilder> ServerLog::_builder = nullptr; ///< 日志器初始化
}