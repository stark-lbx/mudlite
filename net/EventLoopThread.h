// clang-format off
#pragma once

#include "mudlite/base/noncopyable.h"
#include "mudlite/base/mthread.h"


#include <functional>
#include <mutex>
#include <condition_variable>

namespace mudlite
{
namespace net
{

class EventLoop;

class EventLoopThread: noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    MThread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_; // 构造初始化时需要回调一下什么方法
};



} // namespace net    
} // namespace mudlite

