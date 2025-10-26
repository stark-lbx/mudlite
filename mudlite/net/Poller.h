// clang-format off
#pragma once
#include <vector>
#include <unordered_map>

#include "../base/noncopyable.h"
#include "../base/timestamp.h"

namespace mudlite
{
namespace net
{

class EventLoop;
class Channel;

/**
 * muduo库中多路复用分发器的核心IO复用模块
 * 
 * 就是对epoll等IO复用模型的封装, 并且具有所属EventLoop
 * 一个Poller管理多个fd及其事件，也就是Channel
 */
class Poller : noncopyable{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller();

    // 给所有的IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    // 判断参数channel是否在当前poller类中
    bool hasChannel(Channel *channel) const;

    // EventLoop事件循环可以通过该接口获取默认的IO复用的具体实现
    // 并不在源文件中实现, 如果要实现，就需要包含一个EpollPoller.h或PollPoller.h
    // 而这些派生类文件中, 一定还会包含本抽象父类的头文件，就会
    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;

private:
    EventLoop* onwerLoop_;
};

} // namespace net
} // namespace mudlite