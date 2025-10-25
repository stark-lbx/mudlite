# cmake_minimum_required(VERSION 3.22)

set(net_SRCS
 Channel.cc
 EventLoop.cc
 InetAddr.cc
 Poller.cc
 poller/DefaultPoller.cc
 poller/EPollPoller.cc
 TcpServer.cc 
 )

# 编译生成静态库(or动态库)
add_library(mudlite_net ${net_SRCS})

# 让mudlite库链接其它库
target_link_libraries(mudlite_net mudlite_base)

install(TARGETS mudlite_net DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/mudlite/net)