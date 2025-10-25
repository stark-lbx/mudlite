// clang-format off
#pragma once

#include "mudlite/base/noncopyable.h"
#include "mudlite/net/InetAddr.h"
#include "mudlite/net/Channel.h"
#include "mudlite/net/Socket.h"

#include <functional>

namespace mudlite
{
namespace net
{

class EventLoop;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddr&)>;

    Acceptor(EventLoop* loop, const InetAddr& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        newConnectionCallback_ = cb;
    }

    bool listen();
    bool listening() const {return listening_;}
private:
    void handleRead(); // 即新连接到来

    EventLoop *loop_; // Acceptor用的就是用户定义的那个baseLoop，也就是mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_; // 监听事件

    NewConnectionCallback newConnectionCallback_;
    bool listening_;
    int idleFd_;
};



} // namespace net
} // namespace mudlite
