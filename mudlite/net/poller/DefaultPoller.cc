#include "../Poller.h"
#include "EPollPoller.h"

#include <cstdlib>

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