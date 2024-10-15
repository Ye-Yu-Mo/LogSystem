/**
 * @file ServerSink.hpp
 * @brief 定义了 ServerSink 类，用于将日志信息发送到远程服务器
 *
 * 此文件包含 ServerSink 类的声明，该类继承自 Xulog::LogSink，
 * 实现了将日志信息通过 UDP 协议发送到指定的远程服务器的功能。
 *
 */

#include "../logs/Xulog.h"
#include "../server/codec.hpp"
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
class UDPServerSink : public Xulog::LogSink
{
public:
    /// @brief 服务器落地类
    /// @param serverip 服务器ip地址
    /// @param serverport 服务器端口号
    /// @param name 日志器名称
    UDPServerSink(const std::string &serverip, uint16_t serverport, const std::string &name)
        : _serverip(serverip), _serverport(serverport), _logger_name(name)
    {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_sockfd < 0)
        {
            FATAL("socket创建失败");
            exit(errno);
        }
        memset(&_server, 0, sizeof(_server));
        _server.sin_family = AF_INET;
        _server.sin_port = htons(serverport);
        _server.sin_addr.s_addr = inet_addr(serverip.c_str());
    }
    /// @brief 发送数据
    /// @param data 数据指针
    /// @param len 数据长度
    void log(const char *data, size_t len)
    {
        if (_logger == nullptr)
            _logger = Xulog::getLogger(_logger_name);
        if (_logger == nullptr)
            std::cout << "未获取到日志器！日志器名称为：" << _logger_name << std::endl;
        Xulog::LogMsg msg = _logger->getMsg();
        Json::StreamWriterBuilder writer;
        std::string jsonString = Json::writeString(writer, Xulog::Codec::toJson(msg, _logger));
        sendto(_sockfd, jsonString.c_str(), jsonString.size(), 0, (struct sockaddr *)&_server, sizeof(_server));
    }

    ~UDPServerSink() {}

private:
    int _sockfd;                       ///< UDP 套接字文件描述符
    struct sockaddr_in _server;        ///< 远程服务器的地址信息
    std::string _serverip;             ///< 远程服务器的 IP 地址
    uint16_t _serverport;              ///< 远程服务器的端口号
    std::string _logger_name;          ///< 日志器名称
    Xulog::LogMsg _msg;                ///< 结构化数据
    static Xulog::Logger::ptr _logger; ///< 日志器句柄
};
Xulog::Logger::ptr UDPServerSink::_logger = nullptr;