// clang-format off
#pragma once

#include "mudlite/base/noncopyable"

#include <functional>
#include <memory>
#include <thread>
#include <string>
#include <atomic>

namespace mudlite
{

class MThread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit MThread(ThreadFunc, const std::string& name = std::string());
    ~MThread();

    void start();
    void join();

    bool started() const {return started_;}
    int tid() const { return tid_; }
    const std::string& name() const {return name_;}

    static int numCreated(){ return numCreated_.load(); }

private:
    void setDefaultName();

    bool started_;
    pid_t tid_;
    std::shared_ptr<std::thread> thread_;

    ThreadFunc func_;
    std::string name_;

    static std::atomic<int> numCreated_;
}


} // mudlite