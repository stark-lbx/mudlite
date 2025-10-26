#include <cstring>

#include "InetAddr.h"

using namespace mudlite;
using namespace mudlite::net;

InetAddr::InetAddr(uint16_t port, const std::string &ip)
{
    // 初始化 sockaddr_in 结构体
    ::bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = ::htons(port);

    ::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
}

std::string InetAddr::toIp() const
{
    char ip[INET_ADDRSTRLEN] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, ip, sizeof(ip));
    return std::string(ip);
}

std::string InetAddr::toIpPort() const
{
    char ip[INET_ADDRSTRLEN] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, ip, sizeof(ip));
    uint16_t port = ::ntohs(addr_.sin_port);

    return std::string(ip) + ":" + std::to_string(port);
}

uint16_t InetAddr::toPort() const
{
    return ::ntohs(addr_.sin_port);
}

// // for test
// #include <iostream>
// int main()
// {
//     InetAddr addr(8080);
//     std::cout << addr.toIpPort() << std::endl;
//     return 0;
// }