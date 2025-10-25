#include "mudlite/net/poller/EPollPoller.h"

#include "mudlite/base/logger.h"
#include "mudlite/net/Channel.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

using namespace mudlite;
using namespace mudlite::net;

// state
namespace
{
    const int kNew = -1;    // 未添加到Poller中
    const int kAdded = 1;   // 已添加到Poller中
    const int kDeleted = 2; // 已从Poller中删除
}
// channel 的成员 index_ = -1默认

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("EPollPoller::EPollPoller");
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // epoll_wait
    LOG_INFO("fd total count = %d", channels_.size());
    int numEvents = ::epoll_wait(epollfd_,
                                 &(*events_.begin()), // events_.data(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);

    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        // 正常
        LOG_INFO("%d events happened", numEvents);
        fillActiveChannels(numEvents, activeChannels);

        // 扩容
        if (size_t(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_INFO("nothing happened");
    }
    else
    {
        // error happens, log uncommon ones
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_FATAL("EPollPoller::poll() err");
        }
    }
    return now;
}

// 就是epoll_wait后从内核就绪链表中拿到的Event，给填充到activeChannels中
// 这里的函数名称设计，更形象一点就是getActiveChannel，但因为第二个参数是传出参数
// 所以就是从Poller类中缓存的EventList中取出前numEvents个
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels)
{
    numEvents = std::min(numEvents, int(events_.size()));
    // 填充numEvents个活跃的Channel
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}


void EPollPoller::updateChannel(Channel *channel)
{
    // 通过channel修改事件
    int index = channel->index();
    if (index == kNew || index == kDeleted)
    {
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if (index == kNew)
        {
            channels_[fd] = channel;
        }
        else // index == kDeleted
        {
        }

        // 修改为已添加
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        (void)fd;
        if (channel->isNoneEvent())
        {
            // 不关心任何事件, 删除
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            // 修改
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel *channel)
{
    // EPOLL_CTL_DEL channel->fd
    int fd = channel->fd();
    LOG_INFO("fd = %d", fd);

    int index = channel->index();

    channels_.erase(fd);

    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 封装epoll_ctl
void EPollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl op = EPOLL_CTL_DEL, fd=%d", fd);
        }
        else
        {
            LOG_FATAL("epoll_ctl op = EPOLL_CTL_%d, fd=%d", operation, fd);
        }
    }
}