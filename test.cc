#include "util.hpp"
#include "level.hpp"
int main()
{
    // 测试工具类
    // std::cout << Xulog::Util::Date::getTime() << std::endl;
    // std::string pathname = "./dir1/dir2/";
    // Xulog::Util::File::createDirectory(Xulog::Util::File::path(pathname));

    // 测试等级类
    std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::DEBUG) << std::endl;
    std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::INFO) << std::endl;
    std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::WARN) << std::endl;
    std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::ERROR) << std::endl;
    std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::FATAL) << std::endl;
    std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::OFF) << std::endl;
    std::cout << Xulog::LogLevel::toString(Xulog::LogLevel::value::UNKNOW) << std::endl;
    return 0;
}