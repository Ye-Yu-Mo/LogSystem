#include "../logs/Xulog.h"
#include <cstring>
void test_log(const std::string& name)
{
    Xulog::Logger::ptr logger = Xulog::getLogger(name);

    DEBUG("%s","测试开始");
    debug(logger, "%s...", "debug");
    info(logger, "%s...", "info");
    warn(logger, "%s...", "warn");
    error(logger, "%s...", "error");
    fatal(logger, "%s...", "fatal");
    // for(int i=0 ; i<50000; i++)
    // {
    //     fatal(logger, "%s-%d", "测试", i);
    // }
    FATAL("%s","测试结束");

}
int main()
{
    std::unique_ptr<Xulog::LoggerBuilder> builder(new Xulog::GlobalLoggerBuild());
    builder->buildLoggerLevel(Xulog::LogLevel::value::DEBUG);
    builder->buildLoggerName("synclogger");
    builder->buildFormatter("[%d{%y-%m-%d|%H:%M:%S}][%c][%f:%l][%p]%T%m%n");
    builder->buildLoggerType(Xulog::LoggerType::LOGGER_SYNC);
    builder->buildSink<Xulog::StdoutSink>(Xulog::StdoutSink::Color::Enable);
    builder->buildSink<Xulog::FileSink>("./log/test.log");
    builder->buildSink<Xulog::RollSinkBySize>("./log/roll-", 1024 * 1024);
    builder->build();
    test_log("synclogger");
    return 0;
}