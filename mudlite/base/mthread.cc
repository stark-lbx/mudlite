#include "mthread.h"

#include "currthread.h"

#include <semaphore.h>

using namespace mudlite;
// using mudlite::MThread;

std::atomic<int> MThread::numCreated_{0};

MThread::MThread(ThreadFunc func, const std::string &n)
    : started_(false),
      tid_(0),
      thread_(nullptr),
      func_(std::move(func)),
      name_(n)
{
    setDefaultName();
}

MThread::~MThread()
{
    // thread_ != nullptr 代表 started_ == true
    // thread_->joinable() 代表 started_ == true && joined_ == false
    if (started_ && thread_->joinable())
    {
        thread_->detach();
    }
}

void MThread::start()
{
    started_ = true;
    sem_t sem;
    ::sem_init(&sem, false, 0);
    thread_ = std::make_shared<std::thread>(
        [&]
        {
            // 获取线程ID
            tid_ = currthread::tid();
            ::sem_post(&sem);
            // 开启一个新线程执行该线程函数
            func_();
        });
    
    // 这里必须等待获取上面新创建的线程的tid值
    ::sem_wait(&sem); // 等待thread_执行到sem_post(), 确保tid已经分配
}

void MThread::join()
{
    thread_->join();
}

void MThread::setDefaultName()
{
    int num = numCreated_.fetch_add(1);
    if (name_.empty())
    {
        char buf[32];
        ::snprintf(buf, sizeof buf, "MThread%d", num);
        name_ = buf;
    }
}