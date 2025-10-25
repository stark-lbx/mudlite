// clang-format off
#pragma once

#include <sys/socket.h>

#include "mudlite/base/noncopyable.h"
#include "mudlite/net/InetAddr.h"

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
    void accept(InetAddr* peeraddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};




} // namespace net
} // namespace mudlite