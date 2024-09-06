#include "util.hpp"

int main()
{
    std::cout << Xulog::Util::Date::getTime() << std::endl;
    std::string pathname = "./dir1/dir2/";
    Xulog::Util::File::createDirectory(Xulog::Util::File::path(pathname));

    return 0;
}