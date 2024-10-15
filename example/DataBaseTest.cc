#include "../logs/Xulog.h"
#include "../extend/DataBaseSink.hpp"

int main()
{
    std::string format = "[%d{%y-%m-%d|%H:%M:%S}][%f:%l]%T%m%n";
    auto builder = std::make_shared<Xulog::GlobalLoggerBuild>();
    builder->buildLoggerLevel(Xulog::LogLevel::value::DEBUG);
    builder->buildLoggerName("Asynclogger");
    builder->buildFormatter(format);
    builder->buildLoggerType(Xulog::LoggerType::LOGGER_ASYNC);
    builder->buildSink<Xulog::StdoutSink>(Xulog::StdoutSink::Color::Enable);
    builder->buildSink<DataBaseSink>("./data/log.db", "Asynclogger");
    builder->build();
    auto logger = Xulog::getLogger("Asynclogger");
    warn(logger, "warn");
    debug(logger, "debug");
    info(logger, "info");
    fatal(logger, "fatal");
    return 0;
}