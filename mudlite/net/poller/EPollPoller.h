// clang-format off
#pragma once
#include "../Poller.h"

using namespace mudlite;
using namespace mudlite::net;

struct epoll_event;

namespace mudlite
{
namespace net
{

///
/// IO Multiplexing with epoll
///
class EPollPoller : public Poller{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    static const int kInitEventListSize = 16;

    // static const char* operationToString(int op);

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    // 更新channel
    void update(int operation, Channel* channel);

    using EventList= std::vector<struct epoll_event>;

    int epollfd_;
    EventList events_; // 接收epoll_wait的就绪事件的缓冲区
};


} // net
} // namespace mudlite
