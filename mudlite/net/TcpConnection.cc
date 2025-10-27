#include "../base/logger.h"
#include "../base/timestamp.h"

#include "TcpConnection.h"
#include "EventLoop.h"
#include "Channel.h"
#include "InetAddr.h"
#include "Buffer.h"
#include "Socket.h"

#include <memory>
#include <string.h>
#include <errno.h>

using namespace mudlite;
using namespace mudlite::net;

namespace
{
    static EventLoop *CheckLoopNotNull(EventLoop *loop)
    {
        if (loop == nullptr)
        {
            LOG_FATAL("%s:%s:%d tcpConnection loop is null! \n", __FILE__, __func__, __LINE__);
        }
        return loop;
    }
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InetAddr &localAddr,
                             const InetAddr &peerAddr)
    : loop_(CheckLoopNotNull(loop)),
      name_(name),
      state_(kConnecting),
      reading_(true),
      socket_(std::make_unique<Socket>(sockfd)),
      channel_(std::make_unique<Channel>(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024)
{

    // 给channel设置相应的回调函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    LOG_DEBUG("TcpConnection::ctor[%s] at %p fd= %d\n", name_.c_str(), this, sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at %p fd= %d\n", name_.c_str(), this, channel_->fd());
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        // 已建立连接的用户, 有可读事件发生了, 调用用户传入的回调操作：
        // onMessage()
        messageCallback_(
            shared_from_this(),
            &inputBuffer_,
            receiveTime);
    }
    else if (n == 0)
    {
        // 连接断开
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                // 发送完了
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_FATAL("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("Connection fd = %d is down, no more writing", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d state=%d \n", channel_->fd(), state_.load());
    state_ = kDisconnected; // 设置状态
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 执行连接关闭的回调
    closeCallback_(connPtr);      // 关闭连接的回调
}

void TcpConnection::handleError()
{
    int err = Socket::getSocketError(channel_->fd());
    LOG_ERROR("TcpConnection::handleError [%s] - SO_ERROR = %d : %s", name_.c_str(), err, ::strerror(err));
}

/**
 * 发送数据：应用写的快，而内核发送数据慢
 * 需要把待发送的数据写入缓冲区，而且设置了水位回调
 */
void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::sendInLoop(const void *message, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing");
        return;
    }
    // if no thing in output queue, try writing directly
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), message, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // 一次性写完
                loop_->queueInLoop(std::bind(
                    writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nrote<0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_FATAL("TcpConnection::sendInloop");
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    // 这一次write并没有把数据全部发出去，剩余的数据需要保存到缓冲区
    // 注册epollout事件，poller发现tcp的发送缓冲区有空间，会通知相应的sock-channel
    if (!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_,
                                         shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char *>(message) + nwrote, remaining);
        if (!channel_->isWriting())
        {
            // 注册channel的写事件
            // 否则poller不会通知epollout事件
            channel_->enableWriting();
        }
    }
}

// 连接建立
void TcpConnection::connectEstablished()
{
    state_ = kConnected;
    channel_->tie(shared_from_this());
    channel_->enableReading();
    // channel 解决的问题：事件回调时对象可能会被销毁
    // 在建立连接时绑定：此时刚刚创建，尚未开始处理事件
    // 然后enableReading开启读事件的监听

    // 新连接建立, 执行回调
    connectionCallback_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestoryed()
{
    int excepted = kConnected;
    int newvalue = kDisconnected;
    if(state_.compare_exchange_weak(excepted, newvalue))
    {
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

// 关闭当前连接
void TcpConnection::shutdown()
{
    // compare and swap[CAW]
    if(state_ == kConnected)
    {
        state_ = kDisconnecting;
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }
}