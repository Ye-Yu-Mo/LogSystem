#include "../logs/Xulog.h"

void test_log(const std::string& name)
{
    Xulog::Logger::ptr logger = Xulog::getLogger(name);

    DEBUG("%s", "测试开始");
    debug(logger, "%s", "测试全局接口");
    info(logger, "%s", "测试全局接口");
    error(logger, "%s", "测试全局接口");
    warn(logger, "%s", "测试全局接口");

    for(int i=0 ; i<50000; i++)
    {
        fatal(logger, "%s-%d", "测试", i);
    }

    INFO("%s", "测试结束");
}

int main()
{
    std::unique_ptr<Xulog::LoggerBuilder> builder(new Xulog::GlobalLoggerBuild());
    builder->buildLoggerLevel(Xulog::LogLevel::value::DEBUG);
    builder->buildLoggerName("Synclogger");
    builder->buildFormatter("[%d{%y-%m-%d|%H:%M:%S}][%c][%f:%l][%p]%T%m%n");
    builder->buildLoggerType(Xulog::LoggerType::LOGGER_SYNC);
    builder->buildSink<Xulog::StdoutSink>();
    builder->buildSink<Xulog::FileSink>("./log/a_test.log");
    builder->buildSink<Xulog::RollSinkBySize>("./log/a_roll-", 1024 * 1024);
    builder->build();
    test_log("Synclogger");
    return 0;
}