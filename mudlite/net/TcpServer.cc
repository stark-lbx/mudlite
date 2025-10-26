#include "../base/logger.h"
#include "TcpServer.h"

using namespace mudlite;
using namespace mudlite::net;

namespace
{
    static EventLoop *CheckLoopNotNull(EventLoop *loop)
    {
        if (loop == nullptr)
        {
            LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __func__, __LINE__);
        }
        return loop;
    }
}

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

// 新建一个连接：由mainLoop的acceptor处理器来调用这个回调
void TcpServer::newConnection(int sockfd, const InetAddr &peerAddr)
{
    /**
     * 当accept得到新的连接之后，应当将 sockfd 这个新的连接 fd 封装为 tcpconnection
     * 然后将 tcpconnection的新连接建立事件 分发给一个 loop 中
     * tcpConnection的新连接建立事件 需要将 connfd 注册成一个channel
     */
    // 轮询策略获取下一个loop
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    InetAddr localAddr{ Socket::getLocalAddr(sockfd) };
    // 即将这个连接分配给ioLoop
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(ioLoop,   // IO的
                                              connName, // 连接名 
                                              sockfd,   // 连接管理的fd
                                              localAddr, // 本端的addr
                                              peerAddr); // 对端的addr
    connections_[connName] = conn; // 连接管理表
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    ioLoop->runInLoop(
        std::bind(&TcpConnection::connectEstablished, conn));
    // runInLoop: 在mainLoop调用的runInLoop, 在里面其实会判断到 ioLoop 的 tid 与 当前的mainLoop的tid不一样
    // 也就是说 ioLoop 调用的其实是 queueInLoop(), 然后因为不在自己的线程内，所以会 wakeup 唤醒 ioLoop的线程
    // 然后ioLoop醒了之后就处理这个新来的连接建立
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
}

// 设置底层subloop的个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// 开启服务器监听
void TcpServer::start()
{
    // 防止一个TcpServer对象被started多次
    if (started_++ == 0)
    {
        // 启动底层的 loop 线程池
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}