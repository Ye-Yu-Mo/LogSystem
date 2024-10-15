#include "../extend/ServerSink.hpp"

int main()
{
    std::unique_ptr<Xulog::LoggerBuilder> builder(new Xulog::GlobalLoggerBuild());
    builder->buildLoggerLevel(Xulog::LogLevel::value::DEBUG);
    builder->buildLoggerName("synclogger");
    builder->buildFormatter("[%d{%y-%m-%d|%H:%M:%S}][%c][%f:%l][%p]%T%m%n");
    builder->buildLoggerType(Xulog::LoggerType::LOGGER_SYNC);
    // builder->buildSink<Xulog::StdoutSink>(Xulog::StdoutSink::Color::Enable);
    builder->buildSink<UDPServerSink>("127.0.0.1", 8888, "synclogger");
    builder->build();
    Xulog::Logger::ptr logger = Xulog::getLogger("synclogger");

    DEBUG("%s", "测试.....");
    debug(logger, "%s...", "...debug");
    info(logger, "%s...", "...info");
    warn(logger, "%s...", "...warn");
    error(logger, "%s...", "...error");
    fatal(logger, "%s...", "...fatal");

    return 0;
}
