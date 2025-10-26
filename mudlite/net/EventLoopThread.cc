#include "EventLoop.h"
#include "EventLoopThread.h"

using namespace mudlite;
using namespace mudlite::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启动循环-开启一个新线程

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }

    return loop_;
}

// 下面的这个方法是在新线程里面执行的
void EventLoopThread::threadFunc()
{
    // one loop per thread
    EventLoop loop; // 创建一个独立的eventLoop, 和上面的线程是一一对应的

    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // poll
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}