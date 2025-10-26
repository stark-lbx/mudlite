// clang-format off
#pragma once 

#include <functional>
#include <memory>

#include "../base/noncopyable.h"
#include "../base/timestamp.h"


/**
 * EventLoop Channel Poller之间的关系:
 * EventLoop 是一个epoll_wait循环
 * Channel 是EventLoop循环过程中注册的fd与events，并设置回调的通道
 * Poller 是epoll的create、ctl的封装 
 */ 

namespace mudlite
{
namespace net
{

// 前置声明
class EventLoop;

/** 
 * Channel 理解为通道，封装了sockfd和其感兴趣的event
 * 如EPOLLIN、EPOLLOUT事件
 * 还绑定了Poller返回的具体事件
 * 
 * 这就是个Event(fd + events + handler), 并有所属EventLoop
 */
class Channel : noncopyable
{
public:
    using EventCallback     = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知之后，处理事件的 (也就是poller主动调用该方法)
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb){readCallback_ = cb;}
    void setWriteCallback(EventCallback cb){writeCallback_ = cb;}
    void setCloseCallback(EventCallback cb){closeCallback_ = cb;}
    void setErrorCallback(EventCallback cb){errorCallback_ = cb;}

    // 防止channel被手动remove掉, channel还在执行回调
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const {return events_;}
    void set_revents(int revt) {revents_ = revt;}

    // 设置fd相应的事件状态
    void enableReading() {events_ |= kReadEvent; update();}
    void disableReading() {events_ &= ~kReadEvent; update();}
    void enableWriting() {events_ |= kWriteEvent; update();}
    void disableWriting() {events_ &= ~kWriteEvent; update();}
    void disableAll() {events_ = kNoneEvent; update();}

    // 返回fd当前的事件状态
    bool isNoneEvent()const {return events_ == kNoneEvent;}
    bool isWriting() const {return events_ & kWriteEvent;}
    bool isReading() const {return events_ & kReadEvent;}

    // for poller: 
    int index() const {return index_;}
    void set_index(int idx) {index_ = idx;}

    // one loop per thread
    EventLoop* ownerLoop(){return loop_;}
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;   // 事件循环
    const int fd_;  // fd, poller监听的对象
    int events_;    // 注册fd感兴趣的事件
    int revents_;    // poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为channel通道能够获知fd最终发生的具体事件revents
    // 所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

} // namespace net
} // namespace mudlite