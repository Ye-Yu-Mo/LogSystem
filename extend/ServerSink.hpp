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
    /// @param name 日志器名称（保留以兼容旧调用，内部不再使用）
    ServerSink(const std::string &serverip, uint16_t serverport, const std::string &name)
        : _send_socket(std::make_shared<XuServer::TcpSocket>()), _server_ip(serverip), _server_port(serverport)
    {
        (void)name;
    }
    /// @brief 发送数据（结构化重载，优先使用调用链传入的 LogMsg）
    /// @param data 数据指针
    /// @param len 数据长度
    /// @param msg 结构化日志消息
    void log(const char *data, size_t len, const Xulog::LogMsg &msg) override
    {
        Json::StreamWriterBuilder writer;
        std::string jsonString = Json::writeString(writer, Xulog::Codec::toJson(msg));
        sendJson(jsonString);
    }
    /// @brief 发送数据（字节兜底：异步路径无结构化数据，按非结构化消息发送）
    /// @param data 数据指针
    /// @param len 数据长度
    void log(const char *data, size_t len) override
    {
        Json::StreamWriterBuilder writer;
        std::string jsonString = Json::writeString(writer, Xulog::Codec::toJson(std::string(data, len)));
        sendJson(jsonString);
    }

    ~ServerSink()
    {
        _send_socket->CloseSockFd();
    }

private:
    void sendJson(const std::string &jsonString)
    {
        uint32_t sz = htonl(static_cast<uint32_t>(jsonString.size()));
        std::vector<char> buffer(sizeof(uint32_t) + jsonString.size());
        ::memcpy(buffer.data(), &sz, sizeof(uint32_t));
        ::memcpy(buffer.data() + sizeof(uint32_t), jsonString.data(), jsonString.size());
        std::string ip = _server_ip;
        _send_socket->BuildConnectSockedMethod(ip, _server_port);
        _send_socket->Send(buffer);
        _send_socket->CloseSockFd();
    }

    std::shared_ptr<XuServer::TcpSocket> _send_socket; ///< TCP socket
    std::string _server_ip;                            ///< 服务器ip
    uint16_t _server_port;                             ///< 服务器port
};