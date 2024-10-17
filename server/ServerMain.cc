#include "../extend/DataBaseSink.hpp"
#include "codec.hpp"
#include "server.hpp"
#include "serverlog.hpp"
std::string logMsg(std::vector<char> &msg, bool *error_code)
{
    XuServer::ServerLog log;
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
        log(jsonData);
        offset += (sizeof(uint32_t) + sz);
    }
    msg.clear();
    return std::string();
}

int main()
{
    std::shared_ptr<XuServer::TcpServer> tps = std::make_shared<XuServer::TcpServer>(8888, logMsg);
    tps->Loop();
    return 0;
}