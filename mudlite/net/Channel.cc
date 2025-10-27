#include <sys/epoll.h>

#include "../base/logger.h"
#include "Channel.h"
#include "EventLoop.h"

using namespace mudlite;
using namespace mudlite::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel() {}

// channel的tie方法什么时候调用?（遗留问题）
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::remove()
{
    // 在channel所属的loop中把当前这个channel删除
    // 一个loop可以包含多个channel
    loop_->removeChannel(this);
}

// 当改变channel所表示的fd的events事件后
// update负责poller里面更改fd相应的事件epoll_ctl
void Channel::update()
{
    // 通过channel所属的eventloop调用poller相应的方法，注册fd的events事件
    loop_->updateChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;
    if (tied_)
    {
        guard = tie_.lock();
        if (guard)
        { // 提升成功
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的具体事件, 由channel负责调用回调事件
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("Channel[%d] handleEvent revents:%d\n", fd_, revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }

    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }

        if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}
