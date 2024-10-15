/**
 * @file UDPserver.hpp
 * @brief 定义了 UdpServer 类，用于实现 UDP 服务器功能
 *
 * 此文件包含 UdpServer 类的声明，该类实现了一个简单的 UDP 服务器。
 * 主要功能包括创建 UDP 套接字、绑定端口、接收数据以及日志记录。
 *
 */

#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nocopy.hpp"
#include "InetAddr.hpp"
#include "../logs/Xulog.h"
#include "../extend/DataBaseSink.hpp"
#include "codec.hpp"

namespace XuServer
{

    const static uint16_t defaultport = 8888; ///< 默认端口号
    const static uint16_t defaultfd = -1;     ///< 默认socket
    const static uint16_t defaultsize = 1024; ///< 默认缓冲区大小

    /// @brief UDP 服务器类
    class UdpServer : public nocopy // 服务器不允许被拷贝，简化的单例模式
    {
    public:
        /// @brief 构造函数
        /// @param port 监听端口
        UdpServer(uint16_t port = defaultport) : _port(port) {}
        /// @brief 初始化服务器
        void Init()
        {
            // 创建socket文件描述符，对其进行初始化
            _sockfd = socket(AF_INET, SOCK_DGRAM, 0); // 使用IPv4，UDP协议
            if (_sockfd < 0)
            {
                FATAL("socket创建失败!");
                exit(errno);
            }
            struct sockaddr_in local;
            bzero(&local, sizeof(local));       // memset
            local.sin_family = AF_INET;         // 协议簇
            local.sin_port = htons(_port);      // 端口号
            local.sin_addr.s_addr = INADDR_ANY; // ip地址
            int n = bind(_sockfd, (struct sockaddr *)&local, sizeof(local));
            if (n != 0)
            {
                FATAL("bind出错");
                exit(errno);
            }
        }
        /// @brief 启动服务器
        void Start() // 服务器不退出
        {
            char buffer[1024];
            /// 初始化日志建造者
            std::unique_ptr<Xulog::LoggerBuilder> builder(new Xulog::GlobalLoggerBuild());
            builder->buildLoggerName("server");
            builder->buildFormatter();
            builder->build();
            Xulog::Logger::ptr logger = Xulog::getLogger("server");

            for (;;)
            {
                struct sockaddr_in peer; // 远程地址信息，因为需要返回请求
                socklen_t len = sizeof(peer);
                ssize_t n = recvfrom(_sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&peer, &len); // 获取从从外部接收的数据
                                                                                                              // 第一个参数是套接字文件描述符，标识接收数据的套接字
                                                                                                              // 第二个参数是接收数据的缓冲区
                if (n > 0)                                                                                    // 正确接收
                {
                    // 接收数据 反序列化
                    std::string tmp(buffer);
                    Json::CharReaderBuilder reader;
                    Json::Value root;
                    std::istringstream(tmp) >> root;
                    Xulog::DeliverMsg msg = Xulog::Codec::fromJson(root);
                    // 格式化日志信息
                    std::string str = builder->getFormatter()->Format(msg.msg);
                    std::cout << str << std::endl;
                    // TODO 构造日志落地器和进行落地
                    Xulog::StdoutSink std(Xulog::StdoutSink::Color::Enable);
                    Xulog::FileSink file("./log/test.log");
                    DataBaseSink db("./data/log", "server");
                    std.log(str.c_str(), str.size());
                    file.log(str.c_str(), str.size());
                    db.log(msg.msg);
                }
            }
        }
        ~UdpServer() {}

    private:
        uint16_t _port; ///< 端口号
        int _sockfd;    ///< socket文件描述符
    };
}