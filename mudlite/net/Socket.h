// clang-format off
#pragma once

#include <sys/socket.h>
#include <strings.h>

#include "../base/noncopyable.h"
#include "../base/logger.h"
#include "InetAddr.h"

namespace mudlite{
namespace net
{

class Socket : noncopyable {
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd) { } 
    
    ~Socket();

    int fd() const {return sockfd_;}
    void bindAddress(const InetAddr& localaddr);
    void listen();
    int accept(InetAddr* peeraddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

    static int getSocketError(int sockfd)
    {
        int optval;
        socklen_t optlen = sizeof optval;

        if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
        {
            return errno;
        }
        return optval;
    }
    static struct sockaddr_in getLocalAddr(int sockfd)
    {
        struct sockaddr_in localaddr;
        ::bzero(&localaddr, sizeof localaddr);
        socklen_t addrlen = sizeof localaddr;
        if(::getsockname(sockfd, (struct sockaddr*)&localaddr, &addrlen) < 0)
        {
            LOG_FATAL("Socket::getLocalAddr");
        }
        return localaddr;
    }
private:
    const int sockfd_;
};




} // namespace net
} // namespace mudlite