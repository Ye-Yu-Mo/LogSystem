/// @file server.hpp
/// @brief TCP服务器
#pragma once
#include "Socket.hpp"
#include "threadpool.hpp"
#include "nocopy.hpp"
#include <iostream>
#include <functional>

namespace XuServer
{
    /// @brief 服务器回调函数
    using CallBack = std::function<std::string(std::vector<char> &, bool *error_code)>;

    class TcpServer;
    /// @class ThreadData
    /// @brief 线程信息
    class ThreadData
    {
    public:
        /// @brief 构造函数
        /// @param tcp_this  服务器指针
        /// @param sockp socket指针
        ThreadData(TcpServer *tcp_this, TcpSocket *sockp)
            : _this(tcp_this), _sockp(sockp) {}

    public:
        TcpServer *_this;  ///< 服务器指针
        TcpSocket *_sockp; ///< socket指针
    };

    /// @class TcpServer
    /// @brief TCP服务器
    class TcpServer : public nocopy
    {
    public:
        /// @brief 构造函数
        /// @param port 监听端口号
        /// @param call_back 处理获取数据的回调函数
        /// @param thread_count 线程数量
        TcpServer(uint16_t port, CallBack call_back, int thread_count = 5)
            : _port(port), _listen_socket(new TcpSocket()),
              _call_back(call_back), _thread_pool(std::make_unique<threadpool>(thread_count))
        {
            _listen_socket->BuildListenSocketMethod(port);
        }
        /// @brief 启动线程
        static void *ThreadRun(void *args)
        {
            std::vector<char> in_buf_stream(1024 * 10 + sizeof(uint32_t));
            ThreadData *td = static_cast<ThreadData *>(args);
            while (true)
            {
                bool ok = true;
                if (!td->_sockp->Recv(&in_buf_stream))
                    break;

                std::string send_string = td->_this->_call_back(in_buf_stream, &ok);
                if (ok)
                    if (!send_string.empty())
                    {
                        std::vector<char> sd(send_string.begin(), send_string.end());
                        td->_sockp->Send(sd);
                    }
                    else
                        break;
            }
            td->_sockp->CloseSockFd();
            delete td->_sockp;
            delete td;
            return nullptr;
        }
        /// @brief 启动监听
        void Loop()
        {
            while (true)
            {
                std::string peer_ip;
                uint16_t peer_port;
                TcpSocket *newsock = _listen_socket->AcceptConnection(&peer_ip, &peer_port);
                if (newsock == nullptr)
                    continue;
                // std::cout << "get a new connection,sockfd:" << newsock->GetSockFd() << "client info:" << peer_ip << ":" << peer_port << std::endl;
                ThreadData *td = new ThreadData(this, newsock);
                _thread_pool->push(ThreadRun, td);
            }
        }

    private:
        int _port;                                ///< 端口号
        TcpSocket *_listen_socket;                ///< 监听socket
        std::unique_ptr<threadpool> _thread_pool; ///< 线程池

    public:
        CallBack _call_back; ///< 回调函数
    };
}