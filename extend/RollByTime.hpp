

// 扩展功能： 滚动文件（时间）
// 1. 以时间段滚动
// 2. time(nullptr)%gap;
#include "../logs/Xulog.h"
#include <unistd.h>
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

//int main()
//{
//    std::unique_ptr<Xulog::LoggerBuilder> builder(new Xulog::GlobalLoggerBuild());
//    builder->buildLoggerLevel(Xulog::LogLevel::value::DEBUG);
//    builder->buildLoggerName("Synclogger");
//    builder->buildFormatter();
//    builder->buildLoggerType(Xulog::LoggerType::LOGGER_SYNC);
//    builder->buildSink<Xulog::StdoutSink>();
//    builder->buildSink<RollSinkByTime>("./log/a_roll-", TimeGap::GAP_SECOND);
//    Xulog::Logger::ptr logger = builder->build();
//    size_t cur = Xulog::Util::Date::getTime();
//
//    while (Xulog::Util::Date::getTime() < cur + 5)
//    {
//        debug(logger, "%s", "时间功能测试");
//        usleep(1000);
//    }
//    return 0;
//}