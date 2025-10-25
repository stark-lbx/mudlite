#include "mudlite/net/Socket.h"
#include "mudlite/base/logger.h"

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
    const auto *addr = localaddr.getSockAddr();
    int ret = ::bind(sockfd_, addr, sizeof(*addr));
    if (ret < 0)
    {
        LOG_FATAL("bind error");
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

void Socket::accept(InetAddr *peeraddr)
{
    struct sockaddr_in addr;
    socklen_t socklen = sizeof addr;
    bzero(&addr, sizeof addr);

    int connfd = ::accept(sockfd_, &addr, &len);
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
    int ret = ::sutdown(sockfd_, SHUT_WR);
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