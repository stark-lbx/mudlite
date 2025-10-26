// clang-format off
#pragma once

// for int64_t
#include <cstdint>
// for std::string
#include <string>

namespace mudlite
{

class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t SinceEpoch_ms);
    static Timestamp now();
    std::string toString();

private:
    int64_t microSecondSinceEpoch_;
};

} // namespace mudlite