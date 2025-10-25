#include "mudlite/net/EventLoopThreadPool.h"
#include "mudlite/net/EventLoopThread.h"

#include <cstdio>

using namespace mudlite;
using namespace mudlite::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop),
      name_(nameArg),
      started_(false),
      numThreads_(0),
      next_(0)
{
    // do nothing
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // baseloop is stack variable, don't to delete or free
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb = ThreadInitCallback())
{
    started_ = true;
    for (int i = 0; i < numThreads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::make_unique<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
    }

    // 如果服务端只有一个线程, 运行baseLoop
    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}

// 工作在多线程中, baseLoop默认以轮询的方式分配channel给subloop
EventLoop *EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = baseLoop_;

    if (!loops_.empty())
    {
        loop = loop_[next_];
        // next_ = (next_ + 1) % loop_.size();
        ++next_;
        if (next_ >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }

    return {}; // to ensure return must be executed!
}