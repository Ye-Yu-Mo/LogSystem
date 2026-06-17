#include "../extend/DataBaseSink.hpp"
#include "../extend/RollByTime.hpp"
#include "config.hpp"
#include "codec.hpp"
#include "server.hpp"
#include "serverlog.hpp"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage :\n\t" << argv[0] << " config_file_path\n";
        return 1;
    }
    XuServer::Config::ptr config = XuServer::Config::getInstance(argv[1]);
    int port = std::stoi(config->get("Server", "port"));
    std::shared_ptr<XuServer::TcpServer> tps = std::make_shared<XuServer::TcpServer>(port, XuServer::ServerLog::logMsg, 1);
    tps->Loop();
    return 0;
}