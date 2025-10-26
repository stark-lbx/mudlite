// clang-format off
#pragma once

#include "../base/timestamp.h"
#include "../base/noncopyable.h"
#include "Callbacks.h"
#include "InetAddr.h"
#include "Buffer.h"

#include <atomic>

namespace mudlite
{
namespace net
{

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer -> 通过设置一个 mainLoop，注册 Acceptor 的 newConnection 处理 ->
 * 当有新用户连接时, acceptor 触发回调：handleRead() -> 通过 socket 的 accept 拿到 connfd ->
 * 通过注册好的newConnection ，轮询(getNextLoop)选择subLoop，-> 注册回调: TcpConnection::connectionEstablish
 * -> 通过建立连接，
 * -> 此时subLoop就监听上Channel了 -> EventLoop在loop中通过epoll_wait监听 -> 当channel的fd有事件发生 -> channel触发回调handleEvent
 * -> 
 */

class TcpConnection : noncopyable, 
                      public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                  const std::string &name,
                  int sockfd,
                  const InetAddr &localAddr,
                  const InetAddr &peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const {return loop_;}
    const std::string& name() const {return name_;}
    const InetAddr& localAddress() const {return localAddr_;}
    const InetAddr& peerAddress() const {return peerAddr_;}

    bool connected() const {return state_ == kConnected;}
    bool disconnected() const {return state_ == kDisconnected;}

    void setConnectionCallback(const ConnectionCallback& cb)
    {connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback& cb)
    {messageCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {writeCompleteCallback_ = cb;}
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb)
    {highWaterMarkCallback_ = cb;}
    void setCloseCallback(const CloseCallback& cb)
    {closeCallback_ = cb;}

    // 发送数据
    void send(const std::string &buf);
    // 关闭当前连接
    void shutdown();

    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestoryed();

private:
    enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting};

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    // TcpConnection都是mainLoop为了维护subLoop的
    // loop_ 里面包含很多TcpConnection
    // 而一个TcpConnection只属于这一个loop_
    EventLoop* loop_; 
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    
    const InetAddr localAddr_;
    const InetAddr peerAddr_;

    ConnectionCallback connectionCallback_; //有新连接到来时的回调
    MessageCallback messageCallback_; // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息返送完毕之后的回调
    CloseCallback closeCallback_;
    HighWaterMarkCallback highWaterMarkCallback_; // 水位控制, 避免对端发送过快而本端接收慢的情况

    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_; // notes: use list<Buffer> as outputBuffer
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

} // namespace net 
} // namespace mudlite