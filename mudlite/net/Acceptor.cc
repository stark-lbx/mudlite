#include "Acceptor.h"
#include "EventLoop.h"
#include "../base/logger.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace mudlite;
using namespace mudlite::net;

static int createNonBlockingSocketFd()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __func__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddr &listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(::createNonBlockingSocketFd()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listening_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReuseAddr(reuseport);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(
        std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen()
{
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    InetAddr peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            // 轮询找到subloop, 唤醒 分发当前的新客户端的Channel
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            // 这个客户端无法被服务，因为没有连接到来的回调
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("in Acceptor::handleRead");
        if (errno == EMFILE)
        {
            // 不理解为啥这么处理（单台服务器已经不足以支撑太多fd了）
            // mudlite:
            LOG_ERROR("%s:%s:%d sockfd created limited:! \n", __FILE__, __func__, __LINE__);

            // muduo:
            // Read the section named "The special problem of
            // accept()ing when you can't" in libev's doc.
            // By Marc Lehmann, author of libev.
            // ::close(idleFd_);
            // idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
            // ::close(idleFd_);
            // idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
