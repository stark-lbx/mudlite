#include "mudlite/net/Poller.h"
#include "mudlite/net/Channel.h"

using namespace mudlite;
using namespace mudlite::net;

Poller::Poller(EventLoop *loop)
    : onwerLoop_(loop) {}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel* channel) const
{
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}