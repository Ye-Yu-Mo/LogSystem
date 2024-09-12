#include "util.hpp"
#include "level.hpp"
#include "format.hpp"
#include "sink.hpp"
#include "logger.hpp"
#include "buffer.hpp"
// 扩展测试： 滚动文件（时间）
// 1. 以时间段滚动
// 2. time(nullptr)%gap;
enum class TimeGap
{
    GAP_SECOND,
    GAP_MINUTE,
    GAP_HOUR,
    GAP_DAY
};
class RollSinkByTime : public Xulog::LogSink
{
public:
    // 传入文件名时，构造并打开文件，将操作句柄管理起来
    RollSinkByTime(const std::string &basename, TimeGap gap_type)
        : _basename(basename)
    {
        switch (gap_type)
        {
        case TimeGap::GAP_SECOND:
            _gap_size = 1;
            break;
        case TimeGap::GAP_MINUTE:
            _gap_size = 60;
            break;
        case TimeGap::GAP_HOUR:
            _gap_size = 3600;
            break;
        case TimeGap::GAP_DAY:
            _gap_size = 3600 * 24;
            break;
        }
        _current_gap = _gap_size == 1 ? Xulog::Util::Date::getTime() : (Xulog::Util::Date::getTime() % _gap_size);
        std::string filename = createNewFile();
        Xulog::Util::File::createDirectory(Xulog::Util::File::path(filename)); // 创建目录
        _ofs.open(filename, std::ios::binary | std::ios::app);
        assert(_ofs.is_open());
    }
    void log(const char *data, size_t len)
    {
        time_t current = Xulog::Util::Date::getTime();
        if (current % _gap_size != _current_gap)
        {
            std::string filename = createNewFile();
            _ofs.close();
            _ofs.open(filename, std::ios::binary | std::ios::app);
            assert(_ofs.is_open());
        }
        _ofs.write(data, len);
        assert(_ofs.good());
    }

private:
    std::string createNewFile()
    {
        time_t t = Xulog::Util::Date::getTime();
        struct tm lt;
        localtime_r(&t, &lt);
        std::stringstream filename;
        filename << _basename << lt.tm_year + 1900 << lt.tm_mon + 1 << lt.tm_mday << lt.tm_hour << lt.tm_min << lt.tm_sec << ".log";
        return filename.str();
    }

private:
    std::string _basename;
    std::ofstream _ofs;
    size_t _current_gap; // 当前时间段的个数
    size_t _gap_size;    // 间隔大小
};

int main()
{
    // 测试工具类
    // std::cout << Xulog::Util::Date::getTime() << std::endl;
    // std::string pathname = "./dir1/dir2/";
    // Xulog::Util::File::createDirectory(Xulog::Util::File::path(pathname));

    // 测试等级类
    // std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::DEBUG) << std::endl;
    // std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::INFO) << std::endl;
    // std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::WARN) << std::endl;
    // std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::ERROR) << std::endl;
    // std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::FATAL) << std::endl;
    // std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::OFF) << std::endl;
    // std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::UNKNOW) << std::endl;

    // 测试日志格式化模块
    // Xulog::LogMsg msg(Xulog::LogLevel::value::ERROR, 124, "main.cc", "root", "格式化功能测试");
    // Xulog::Formatter fmt1;
    // // Xulog::Formatter fmt2("%c{}");
    // std::string str1 = fmt1.Format(msg);
    // // std::string str2 = fmt2.Format(msg);
    // // std::cout << str1 << str2;

    // // 测试原生日志落地模块
    // // Xulog::LogSink::ptr std_lsp = Xulog::SinkFactory::create<Xulog::StdoutSink>();
    // // Xulog::LogSink::ptr file_lsp = Xulog::SinkFactory::create<Xulog::FileSink>("./log/test.log");
    // // Xulog::LogSink::ptr roll_lsp = Xulog::SinkFactory::create<Xulog::RollSinkBySize>("./log/roll-", 1024 * 1024); // 每个文件1MB
    // Xulog::LogSink::ptr time_lsp = Xulog::SinkFactory::create<RollSinkByTime>("./log/roll-", TimeGap::GAP_SECOND); // 每个文件1s

    // // std_lsp->log(str1.c_str(), str1.size());
    // // file_lsp->log(str1.c_str(), str1.size());
    // // size_t size = 0;
    // // size_t cnt = 0;
    // // while (size < 1024 * 1024 * 100) // 100 个
    // // {
    // //     std::string tmp = std::to_string(cnt++);
    // //     tmp += str1;
    // //     roll_lsp->log(tmp.c_str(), tmp.size());
    // //     size += tmp.size();
    // // }
    // time_t t = Xulog::Util::Date::getTime();
    // while (Xulog::Util::Date::getTime() < t + 3)
    // {
    //     time_lsp->log(str1.c_str(), str1.size());
    // }

    // 测试同步日志器
    // std::string logger_name = "synclog";
    // Xulog::LogLevel::value limit = Xulog::LogLevel::value::WARN;
    // Xulog::Formatter::ptr fmt(new Xulog::Formatter());
    // Xulog::LogSink::ptr std_lsp = Xulog::SinkFactory::create<Xulog::StdoutSink>();
    // Xulog::LogSink::ptr file_lsp = Xulog::SinkFactory::create<Xulog::FileSink>("./log/test.log");
    // Xulog::LogSink::ptr roll_lsp = Xulog::SinkFactory::create<Xulog::RollSinkBySize>("./log/roll-", 1024 * 1024);  // 每个文件1MB
    // Xulog::LogSink::ptr time_lsp = Xulog::SinkFactory::create<RollSinkByTime>("./log/roll-", TimeGap::GAP_SECOND); // 每个文件1s
    // std::vector<Xulog::LogSink::ptr> sinks = {std_lsp, file_lsp, roll_lsp, time_lsp};
    // Xulog::Logger::ptr logger(new Xulog::SyncLogger(logger_name, limit, fmt, sinks));
    // std::string str = "测试同步日志器-";
    // logger->debug(__FILE__, __LINE__, "%s", str.c_str());
    // logger->error(__FILE__, __LINE__, "%s", str.c_str());
    // logger->fatal(__FILE__, __LINE__, "%s", str.c_str());
    // logger->info(__FILE__, __LINE__, "%s", str.c_str());
    // logger->warn(__FILE__, __LINE__, "%s", str.c_str());

    // size_t size = 0;
    // int cnt = 1;
    // while (size < 1024 * 1024 * 10) // 10 个
    // {
    //     logger->fatal(__FILE__, __LINE__, "%s-%d", str.c_str(), cnt++);
    //     size += 20;
    // }

    // 测试建造者模式
    // std::unique_ptr<Xulog::LoggerBuilder> builder(new Xulog::LocalLoggerBuild());
    // builder->buildLoggerLevel(Xulog::LogLevel::value::WARN);
    // builder->buildLoggerName("synclog");
    // builder->buildFormatter();
    // builder->buildLoggerType(Xulog::LoggerType::LOGGER_SYNC);
    // builder->buildSink<Xulog::StdoutSink>();
    // builder->buildSink<Xulog::FileSink>("./log/test.log");
    // builder->buildSink<Xulog::RollSinkBySize>("./log/roll-", 1024 * 1024);
    // builder->buildSink<RollSinkByTime>("./log/roll-", TimeGap::GAP_SECOND);
    // Xulog::Logger::ptr logger = builder->build();

    // std::string str = "测试同步日志器-";

    // logger->debug(__FILE__, __LINE__, "%s", str.c_str());
    // logger->error(__FILE__, __LINE__, "%s", str.c_str());
    // logger->fatal(__FILE__, __LINE__, "%s", str.c_str());
    // logger->info(__FILE__, __LINE__, "%s", str.c_str());
    // logger->warn(__FILE__, __LINE__, "%s", str.c_str());

    // size_t size = 0;
    // int cnt = 1;

    // while (size < 1024 * 1024 * 10) // 10 个
    // {
    //     logger->fatal(__FILE__, __LINE__, "%s-%d", str.c_str(), cnt++);
    //     size += 20;
    // }

    // 测试异步缓冲区
    // 读取文件数据，逐步写入缓冲区，最终将缓冲区写入文件，判断新文件是否与原文件是否一致
    std::ifstream ifs("./log/test.log", std::ios::binary);
    if (ifs.is_open() == false)
        return -1;
    ifs.seekg(0, std::ios::end); // 跳转至文件末尾
    size_t fsize = ifs.tellg();  // 获取文件长度
    ifs.seekg(0, std::ios::beg); // 跳转至文件开头
    std::string body;
    body.resize(fsize);
    ifs.read(&body[0], fsize);
    if (ifs.good() == false)
    {
        std::cout << "read error\n";
        return -1;
    }
    ifs.close();
    Xulog::Buffer buffer;
    for (int i = 0; i < body.size(); i++)
    {
        buffer.push(&body[i], 1);
    }
    std::ofstream ofs("./log/tmp.log", std::ios::binary);
    // 一次写入
    // ofs.write(buffer.begin(), fsize);
    // 逐字节写入
    size_t read_size = buffer.readAbleSize();
    for (int i = 0; i < read_size; i++)
    {
        ofs.write(buffer.begin(), 1);
        buffer.moveReader(1);
    }
    ofs.close();
    return 0;
}