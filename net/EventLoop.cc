// clang-format off
#include <sys/eventfd.h>

#include "mudlite/net/EventLoop.h"
#include "mudlite/base/logger.h"
#include "mudlite/net/Channel"

using namespace mudlite;
using namespace mudlite::net;

namespace
{
    // 防止一个线程创建多个EventLoop    thread_local
    __thread EventLoop *t_loopInThisThread = nullptr;

    // 定义默认的Poller IO复用的超时时间
    const int kPollTimeMs = 10000;

    // 创建wakefd，用来notify唤醒subReactor处理新来的channel
    int createEventfd()
    {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0)
        {
            LOG_FATAL("eventfd error:%d \n", errno);
        }
        return evtfd;
    }
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(currthread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("eventloop created %p in thread %d \n", this, threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("another eventloop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else 
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // enableReading() 会调用update方法，这个wakeupChannel就注册到Poller中了
    // 每一个 EventLoop 都将监听 wakeupchannel 的 epollin 事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8", n);
    }
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_){
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for(Channel* channel : activeChannels_){            
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前eventLoop事件循环需要处理的回调操作
        // mainLoop 实现注册一个回调cb需要让subLoop执行的
        // wakeup subLoop后，执行下面的方法，执行之前mainLoop注册的回调
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping", this);
}

// 退出事件循环
/**
 * 在自己线程中主动退出：quit_=true;
 * 例如：在subLoop中退出mainLoop
 */
void EventLoop::quit()
{
    quit_ = true;
    // There is a chance that loop() just executes while(!quit_) and exits;
    // then EventLoop destructs, then we are accessing an invalid object.
    // Can be fixed using mutex_ in both places.
    if(!isInLoopThread()){
        wakeup(); // 唤醒
    }
}

// 通过 wakeupfd 唤醒 EventLoop
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one){
        LOG_ERROR("EventLoop::wakeup() writes %d bytes instead of 8", n);
    }

    // eventloop在loop后进入while循环，在while循环中，有一个poll
    // poll 是 Poller 对 系统调用的封装
    // 在此处可能会被阻塞，该线程就会进入阻塞
    // 通过别的线程调用wakeup、就会向loop的wakeupfd缓冲区写上数据
    // 此时wakeupChannel就有读事件，线程就从 阻塞 状态解除了。
}

// handleRead(): 处理wakeupfd发过来的信息
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one){
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8", n);
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true; // 正在调用待处理的回调

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_); // 把(对象内)全局pending序列拿走，使用局部序列操作
    }

    for(const Functor& functor : functors)
    {
        functor(); // 执行
    }
    callingPendingFunctors_ = false; // 执行完了
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}


void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())
    {
        // 如果在自己的线程循环，直接执行
        cb();
    }
    else
    {
        // 否则入队等loop执行
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    { // 入队
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    // 如果不在自己线程 或者 正在执行待处理的回调
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        // calling时也需要唤醒，是因为：
        // 当doPendingFunctors完之后，没有其它事件到来，但是有新入队的方法需要执行
        // 为了尽快响应，直接让下次的poll打破阻塞，去执行doPendingFuctors        
        wakeup(); // 唤醒这个loop
    }
}