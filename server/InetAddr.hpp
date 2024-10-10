/**
 * @file InetAddr.h
 * @brief 定义了 InetAddr 类，用于格式化和处理网络地址信息
 *
 * 此文件包含了 InetAddr 类的声明，该类用于将接收到的网络信息格式化为可读的形式。
 * 主要功能包括将网络字节序的 IP 地址转换为点分十进制格式，以及提供对 IP 地址和端口号的访问。
 * 
 * @note 此类依赖于 POSIX 网络编程接口，适用于使用 IPv4 协议的网络应用。
 */
#pragma once
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace XuServer
{
    /// @brief 将接收到的网络信息格式化
    class InetAddr
    {
    public:
        /// @brief 构造函数
        /// @param addr 输入的 sockaddr_in 结构体，包含网络地址信息
        /// 将网络字节序转换为主机字节序，并将 IP 地址转换为可读格式
        InetAddr(struct sockaddr_in &addr)
            : _addr(addr)
        {
            _port = ntohs(_addr.sin_port);   // 将网络字节序转换为主机字节序
            _ip = inet_ntoa(_addr.sin_addr); // 将网络字节序的IP转换为点分十进制的字符串
        }
        /// @brief 获取 IP 地址
        /// @return 格式化后的 IP 地址
        std::string Ip()
        {
            return _ip;
        }
        /// @brief 获取端口号
        /// @return 端口号
        uint16_t Port()
        {
            return _port;
        }
        /// @brief 析构函数
        ~InetAddr() {}

    private:
        std::string _ip;          ///< 存储点分十进制格式的 IP 地址
        uint16_t _port;           ///< 存储端口号
        struct sockaddr_in _addr; ///< 存储原始的 sockaddr_in 结构体
    };
}