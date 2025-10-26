#include "Socket.h"
#include "../base/logger.h"

#include <sys/types.h>
#include <unistd.h>
#include <string.h> // for bzero();
#include <netinet/tcp.h>

using namespace mudlite;
using namespace mudlite::net;

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddr &localaddr)
{
    const struct sockaddr_in addr = *localaddr.getSockAddr();
    LOG_INFO("Socket::bindAddress[%d]", sockfd_);
    int ret = ::bind(sockfd_, (struct sockaddr*)&addr, sizeof addr);
    if (ret < 0)
    {
        LOG_FATAL("|-> bind error[%d : %s]", errno, strerror(errno));
    }
}
void Socket::listen()
{
    int ret = ::listen(sockfd_, SOMAXCONN);
    if (ret < 0)
    {
        LOG_FATAL("listen error");
    }
}

int Socket::accept(InetAddr *peeraddr)
{
    struct sockaddr_in addr;
    socklen_t socklen = sizeof addr;
    bzero(&addr, sizeof addr);

    int connfd = ::accept(sockfd_, (struct sockaddr*)&addr, &socklen);
    if (connfd >= 0)
    {
        if (peeraddr)
            peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    // 关闭写端
    int ret = ::shutdown(sockfd_, SHUT_WR);
    if (ret < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int opt = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof opt);
}

void Socket::setReuseAddr(bool on)
{
    int opt = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
}

void Socket::setReusePort(bool on)
{
    int opt = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof opt);
}

void Socket::setKeepAlive(bool on)
{
    int opt = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof opt);
}