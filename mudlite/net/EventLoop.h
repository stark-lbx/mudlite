// clang-format off
#pragma once

#include "../base/noncopyable.h"
#include "../base/timestamp.h"
#include "../base/currthread.h"

#include <mutex>
#include <functional>
#include <vector>
#include <atomic>
#include <memory>

namespace mudlite
{
namespace net
{

class Channel;
class Poller;

class EventLoop : noncopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const {return pollReturnTime_;}

    // core1
    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    // 把cb放入队列中，唤醒loop所在的线程, 执行cb
    void queueInLoop(Functor cb);
    size_t queueSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pendingFunctors_.size();
    }

    // mainloop 唤醒 subloop
    void wakeup(); 

    // EventLoop的方法 --调用--> Poller的方法 
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const {return threadId_ == currthread::tid();}

private:
    void handleRead();
    void doPendingFunctors(); // pending: 待处理

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;  // 原子操作, 通过CAS实现的
    std::atomic_bool quit_;     // 标识退出loop_
    
    const pid_t threadId_; // 记录当前Loop所在的线程ID

    // 主要作用: 当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop
    // 通过该成员唤醒
    int wakeupFd_; // eventfd()
    std::unique_ptr<Channel> wakeupChannel_;

    Timestamp pollReturnTime_; // poller返回事件channels的时间点
    std::unique_ptr<Poller> poller_; // one poller per loop 

    // 接收就绪事件缓冲区
    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否正在执行的回调操作
    std::vector<Functor> pendingFunctors_;
    mutable std::mutex mutex_; // 互斥锁，保护上面vector容器的线程安全的操作
};

} // net    
} // namespace mudlite