#include "mudlite/base/timestamp.h"

using namespace mudlite;

Timestamp::Timestamp() : microSecondSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t SinceEpoch_ms)
    : microSecondSinceEpoch_(SinceEpoch_ms) {}

Timestamp Timestamp::now() { return Timestamp(time(NULL)); }

std::string Timestamp::toString()
{
    char buf[1024] = {0};
    tm *tm_time = localtime(&microSecondSinceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return std::string(buf);
}

// for test
// #include <iostream>
// int main()
// {
//     std::cout << mudlite::Timestamp::now().toString() << '\n';
//     return 0;
// }