#include <cstdlib>

#include "mudlite/net/Poller.h"
#include "mudlite/net/poller/EPollPoller.h"

using namespace mudlite;
using namespace mudlite::net;

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("mudlite_USE_POLL"))
    {
        return nullptr; // 生成poll的实例
    }
    else
    {
        return new EPollPoller(loop); // 生成epoll的实例
    }
    return nullptr;
}