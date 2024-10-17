/**
 * @file ServerSink.hpp
 * @brief 定义了 ServerSink 类，用于将日志信息发送到远程服务器
 *
 * 此文件包含 ServerSink 类的声明，该类继承自 Xulog::LogSink，
 * 实现了将日志信息通过 UDP 协议发送到指定的远程服务器的功能。
 *
 */
#pragma once
#include "../logs/Xulog.h"
#include "../server/codec.hpp"
#include "../server/Socket.hpp"
#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// 设置远程落地方式
// 后台运行
class ServerSink : public Xulog::LogSink
{
public:
    /// @brief 服务器落地类
    /// @param serverip 服务器ip地址
    /// @param serverport 服务器端口号
    /// @param name 日志器名称
    ServerSink(const std::string &serverip, uint16_t serverport, const std::string &name)
        : _send_socket(std::make_shared<XuServer::TcpSocket>()), _logger_name(name), _server_ip(serverip), _server_port(serverport)
    {
    }
    /// @brief 发送数据
    /// @param data 数据指针
    /// @param len 数据长度
    void log(const char *data, size_t len)
    {
        // std::cout << "data: " << std::string(data) <<std::endl;
        if (_logger == nullptr)
            _logger = Xulog::getLogger(_logger_name);
        if (_logger == nullptr)
            FATAL("日志器获取失败 请检查日志器名称");
        Json::StreamWriterBuilder writer;
        std::string jsonString;
        if (_logger->getLoggerType() == Xulog::LoggerType::LOGGER_ASYNC)
        {
            std::cout << "匹配到异步日志器\n";
            jsonString = Json::writeString(writer, Xulog::Codec::toJson(data));
        }
        else
        {
            std::cout << "匹配到同步日志器\n";
            jsonString = Json::writeString(writer, Xulog::Codec::toJson(_logger->getMsg()));
        }

        uint32_t sz = ::htonl(jsonString.size());
        size_t buffer_len = jsonString.size() + sizeof(uint32_t);
        std::cout << "发送的数据是：" << jsonString.size() << ":" << jsonString << std::endl;
        // char buffer[buffer_len];
        std::vector<char> buffer(sizeof(uint32_t) + jsonString.size());

        ::memcpy(buffer.data(), &sz, sizeof(uint32_t));
        ::memcpy(buffer.data() + sizeof(uint32_t), jsonString.data(), jsonString.size());
        std::string ip = _server_ip;
        _send_socket->BuildConnectSockedMethod(ip, _server_port);
        _send_socket->Send(buffer);
        _send_socket->CloseSockFd();
    }

    /// @brief 析构函数
    ~ServerSink()
    {
        _send_socket->CloseSockFd();
    }

private:
    std::shared_ptr<XuServer::TcpSocket> _send_socket; ///< TCP socket
    std::string _logger_name;                          ///< 日志器名称
    Xulog::LogMsg _msg;                                ///< 结构化数据
    static Xulog::Logger::ptr _logger;                 ///< 日志器句柄
    std::string _server_ip;                            ///< 服务器ip
    uint16_t _server_port;                             ///< 服务器port
};
Xulog::Logger::ptr ServerSink::_logger = nullptr; ///< 日志器句柄