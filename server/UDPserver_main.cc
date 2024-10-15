#include "UDPserver.hpp"
#include <memory>
#include <iostream>

void Usage(std::string proc) // 使用提示
{
    std::cout << "Usage:\n\t" << proc << "local_port\n"
              << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        Usage(argv[0]);
        return 1;
    }

    uint16_t port = std::stoi(argv[1]);
    pid_t pid = fork();
    if (pid < 0)
    {
        std::cerr << "Fork failed." << std::endl;
        return 1;
    }
    else if (pid > 0)
    {
        // 父进程退出
        return 0;
    }
    std::unique_ptr<XuServer::UdpServer> usvr = std::make_unique<XuServer::UdpServer>(port);
    usvr->Init();
    usvr->Start();
    return 0;
}