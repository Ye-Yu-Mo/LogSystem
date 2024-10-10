#pragma once
#include <iostream>

namespace XuServer
{
    class nocopy
    {
    public:
        nocopy() {}
        nocopy(const nocopy &) = delete;
        const nocopy &operator=(const nocopy &) = delete;
        ~nocopy() {}
    };
}