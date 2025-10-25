// clang-format off
#pragma once

#include "mudlite/base/timestamp.h"

#include <memory>
#include <functional>

namespace mudlite
{
namespace net
{
    class Buffer;
    class TcpConnection;

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

    using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;

    using MessageCallback = std::function<void(
        const TcpConnectionPtr&,
        Buffer*,
        Timestamp,
    )>;

} // namespace net 
} // namespace mudlite
