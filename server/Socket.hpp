/// @file Socket.hpp
/// @brief 封装TCP和UDPSocket
#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../logs/Xulog.h"

#define Convert(addr_ptr) ((struct sockaddr *)addr_ptr)

namespace XuServer
{
    constexpr int DEFAULT_SOCKFD = -1;   ///< 默认无效sockfd
    constexpr int DEFAULT_BACKLOG = 128; ///< 默认监听队列长度

    /// @class Socket
    /// @brief 抽象基类，封装基本Socket操作。
    class Socket
    {
    public:
        virtual ~Socket() {}
        /// @brief 创建Socket，派生类需实现该函数。
        virtual void CreateSocketOrDie() = 0;
        /// @brief 绑定Socket，派生类需实现该函数。
        /// @param port 需要绑定的端口号。
        virtual void BindSocketOrDie(uint16_t port) = 0;
    };

    /// @class UdpSocket
    /// @brief UDPSocket类
    class UdpSocket : public Socket
    {
    public:
        /// @brief 构造函数
        /// @param sockfd 可传入已有的
        UdpSocket(int sockfd = DEFAULT_SOCKFD) : _sockfd(sockfd) {}
        ~UdpSocket() {}
        /// @brief 发送数据到指定的目标地址。
        /// @param send_data 要发送的字符串数据。
        /// @param dest_ip 目标IP地址。
        /// @param dest_port 目标端口号。
        void SendTo(const std::vector<char> &send_data, const std::string &dest_ip, uint16_t dest_port)
        {
            struct sockaddr_in dest_addr;
            memset(&dest_addr, 0, sizeof(dest_addr));
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_addr.s_addr = inet_addr(dest_ip.c_str());
            dest_addr.sin_port = htons(dest_port);
            ssize_t ret = sendto(_sockfd, send_data.data(), send_data.size(), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (ret < 0)
                throw std::runtime_error("sendto 失败: " + std::string(strerror(errno)));
        }
        /// @brief 从指定源接收数据。
        /// @param buffer 存储接收到的数据的缓冲区。
        /// @param size 缓冲区大小。
        /// @param src_ip 接收到数据的源IP地址。
        /// @param src_port 接收到数据的源端口号。
        /// @return 返回接收的数据字节数。
        ssize_t RecvFrom(std::vector<char> *buffer, size_t size, std::string *src_ip, uint16_t *src_port)
        {
            std::vector<char> inbuffer(size);
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);
            ssize_t n = recvfrom(_sockfd, inbuffer.data(), size, 0, Convert(&peer), &len);
            if (n < 0)
                throw std::runtime_error("recvfrom 失败: " + std::string(strerror(errno)));
            if (n > 0)
            {
                inbuffer.resize(n);
                buffer->swap(inbuffer);
                *src_ip = inet_ntoa(peer.sin_addr);
                *src_port = ntohs(peer.sin_port);
            }

            return n;
        }

    private:
        /// @brief 创建UDP Socket，若创建失败则终止程序。
        void CreateSocketOrDie() override
        {
            _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (_sockfd < 0)
                throw std::runtime_error("Socket 创建失败: " + std::string(strerror(errno)));
        }
        /// @brief 绑定UDP Socket到指定端口，若绑定失败则终止程序。
        /// @param port 要绑定的端口号。
        void BindSocketOrDie(uint16_t port) override
        {
            struct sockaddr_in local;
            bzero(&local, sizeof(local));       // memset
            local.sin_family = AF_INET;         // 协议簇
            local.sin_port = htons(port);       // 端口号
            local.sin_addr.s_addr = INADDR_ANY; // ip地址
            int n = ::bind(_sockfd, (struct sockaddr *)&local, sizeof(local));
            if (n != 0)
                throw std::runtime_error("Bind 失败: " + std::string(strerror(errno)));
        }

    public:
        /// @brief 创建并绑定Socket。
        /// @param port 要绑定的端口号。
        void CreateBuildSocketMethod(uint16_t port)
        {
            CreateSocketOrDie();
            BindSocketOrDie(port);
        }

    private:
        int _sockfd; ///< Socket文件描述符。
    };
    /// @class TcpSocket
    /// @brief TCPSocket类
    class TcpSocket : public Socket
    {
    public:
        /// @brief 构造函数。
        /// @param sockfd socket文件描述符，默认值为DEFAULT_SOCKFD。
        TcpSocket(int sockfd = DEFAULT_SOCKFD) : _sockfd(sockfd) {}
        ~TcpSocket() {}

    public:
        /// @brief 创建、绑定并监听Socket。
        /// @param port 要绑定的端口号。
        /// @param backlog 监听队列长度，默认值为DEFAULT_BACKLOG。
        void BuildListenSocketMethod(uint16_t port, int backlog = DEFAULT_BACKLOG)
        {
            CreateSocketOrDie();
            BindSocketOrDie(port);
            ListenSocketOrDie(backlog);
        }
        /// @brief 建立TCP客户端连接。
        /// @param serverip 服务器的IP地址。
        /// @param serverport 服务器的端口号。
        /// @return 若连接成功，返回true；否则返回false。
        bool BuildConnectSockedMethod(std::string &serverip, uint16_t serverport)
        {
            CreateSocketOrDie();
            return ConnectServer(serverip, serverport);
        }
        /// @brief 设置已建立连接的Socket。
        /// @param sockfd 已建立连接的Socket描述符。
        void BuildNormalSockMethod(int sockfd)
        {
            SetSockFd(sockfd);
        }

        /// @brief 获取当前Socket描述符。
        /// @return 返回当前的Socket描述符。
        int GetSockFd() { return _sockfd; }
        /// @brief 设置Socket描述符。
        /// @param sockfd 要设置的Socket描述符。
        void SetSockFd(int sockfd) { _sockfd = sockfd; }
        /// @brief 关闭当前Socket。
        void CloseSockFd()
        {
            if (_sockfd > DEFAULT_SOCKFD)
                ::close(_sockfd);
        }
        /// @brief 接收来自对端的数据。
        /// @param buffer 存储接收到的数据的缓冲区。
        /// @param size 缓冲区大小，默认值为10KB。
        /// @return 接收成功返回true，否则返回false。
        bool Recv(std::vector<char> *buffer, int size = 1024 * 10)
        {
            std::vector<char> inbuffer(size);
            ssize_t n = recv(_sockfd, inbuffer.data(), size, 0);
            std::string str(inbuffer.begin(), inbuffer.end());
            if (n == 0)
            {
                INFO("客户端连接已断开！");
                return false;
            }
            else if (n < 0)
            {
                ERROR("revc失败！");
                return false;
            }
            if (n > 0)
            {
                inbuffer.resize(n);
                buffer->swap(inbuffer);
                return true;
            }
            return false;
        }
        /// @brief 向对端发送数据。
        /// @param send_data 要发送的数据。
        void Send(const std::vector<char> &send_data)
        {
            ssize_t ret = send(_sockfd, send_data.data(), send_data.size(), 0);
            if (ret < 0)
                throw std::runtime_error("send 失败: " + std::string(strerror(errno)));
        }
        /// @brief 接收新的TCP连接。
        /// @param peerip 存储对端的IP地址。
        /// @param peerport 存储对端的端口号。
        /// @return 返回新的TcpSocket对象，表示新的连接，失败则返回nullptr。
        TcpSocket *AcceptConnection(std::string *peerip, uint16_t *peerport)
        {
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);
            int newsockfd = ::accept(_sockfd, Convert(&peer), &len);
            if (newsockfd < 0)
                return nullptr;
            *peerport = ntohs(peer.sin_port);
            *peerip = inet_ntoa(peer.sin_addr);
            TcpSocket *s = new TcpSocket(newsockfd);
            return s;
        }

    private:
        /// @brief 创建TCP Socket，若创建失败则终止程序。
        void CreateSocketOrDie() override
        {
            _sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
            if (_sockfd < 0)
                throw std::runtime_error("Socket 创建失败！" + std::string(strerror(errno)));
        }
        /// @brief 绑定TCP Socket到指定端口，若绑定失败则终止程序。
        /// @param port 要绑定的端口号。
        void BindSocketOrDie(uint16_t port) override
        {
            struct sockaddr_in local;
            memset(&local, 0, sizeof(local));
            local.sin_family = AF_INET;
            local.sin_addr.s_addr = INADDR_ANY;
            local.sin_port = htons(port);

            int n = ::bind(_sockfd, Convert(&local), sizeof(local));
            if (n < 0)
                throw std::runtime_error("Bind 失败" + std::string(strerror(errno)));
        }
        /// @brief 监听TCP连接，若监听失败则终止程序。
        /// @param backlog 监听队列长度，默认值为DEFAULT_BACKLOG。
        void ListenSocketOrDie(int backlog = DEFAULT_BACKLOG)
        {
            int n = ::listen(_sockfd, backlog);
            if (n < 0)
                throw std::runtime_error("listen 失败" + std::string(strerror(errno)));
        }
        /// @brief 连接到服务器。
        /// @param serverip 服务器的IP地址。
        /// @param serverport 服务器的端口号。
        /// @return 成功返回true，失败返回false。
        bool ConnectServer(std::string &serverip, uint16_t serverport)
        {
            struct sockaddr_in server;
            memset(&server, 0, sizeof(server));
            server.sin_family = AF_INET;
            server.sin_addr.s_addr = inet_addr(serverip.c_str());
            server.sin_port = htons(serverport);

            int n = ::connect(_sockfd, Convert(&server), sizeof(server));
            if (n == 0)
                return true;
            else
                return false;
        }

    private:
        int _sockfd; ///< Socket文件描述符。
    };
}