#include "mudlite/net/TcpServer.h"
#include "mudlite/base/logger.h"

static EventLoop *CheckLoopNotNull(const EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __func__, __LINE__);
    }
    return loop;
}

using namespace mudlite;
using namespace mudlite::net;

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddr &listenAddr,
                     const std::string &nameArg,
                     Option option)
    : loop_(loop),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, nameArg)),
      connectionCallback_(),
      messageCallback_(),
      nextConnId_(1)
{
    // 当有新用户连接时会执行TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

void TcpServer::newConnection(int sockfd, const InetAddr &peerAddr)
{
    /**
     * 当accept得到新的连接之后，应当将 sockfd 这个新的连接 fd 封装为 channel
     * 然后将 channel 分发到一个 loop 中
     */
    // 轮询策略获取下一个loop
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    
    // InetAddr localAddr();
    TcpConnectionPtr conn(/*new TcpConnection()*/);
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
    // runInLoop: 在mainLoop调用的runInLoop, 在里面其实会判断到 ioLoop 的 tid 与 当前的mainLoop的tid不一样
    // 也就是说 ioLoop 调用的其实是 queueInLoop(), 然后因为不在自己的线程内，所以会 wakeup 唤醒 ioLoop的线程
}