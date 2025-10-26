// clang-format off
#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

namespace mudlite
{
namespace net
{

class InetAddr
{
public:
    explicit InetAddr(const sockaddr_in &addr) : addr_(addr) {}
    explicit InetAddr(uint16_t port = 0, const std::string& ip = "127.0.0.1");

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    void setSockAddr(const sockaddr_in &addr){addr_ = addr;}
    const struct sockaddr *getSockAddr() const { return (struct sockaddr*)(&addr_); }
    
private:
    struct sockaddr_in addr_;
};

} // net
} // namespace mudlite