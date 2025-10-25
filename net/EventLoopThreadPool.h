// clang-format off
#pragma once

#include "mudlite/base/noncopyable.h"
// #include "mudlite/net/EventLoopThread.h"


#include <memory>
#include <vector>
#include <functional>

namespace mudlite
{
namespace net
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback  = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // 工作在多线程中, baseLoop默认以轮询的方式分配channel给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const {return started_;}

    const std::string name() const {return name_;}

private:
    EventLoop* baseLoop_; // 用户线程
    std::string name_;
    bool started_;
    int numThreads_; // 线程数目
    int next_;

    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
}



} // namespace nets
} // namespace mudlite
