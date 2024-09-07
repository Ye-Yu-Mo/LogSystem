#include "util.hpp"
#include "level.hpp"
#include "format.hpp"
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
    Xulog::LogMsg msg(Xulog::LogLevel::value::ERROR, 124, "main.cc", "root", "格式化功能测试");
    Xulog::Formatter fmt1;
    Xulog::Formatter fmt2("%c{}");
    std::string str1 = fmt1.Format(msg);
    std::string str2 = fmt2.Format(msg);
    std::cout << str1 << str2;
    return 0;
}