# cmake_minimum_required(VERSION 3.22)

set(base_SRCS
 timestamp.cc
 logger.cc
 currthread.cc
 mthread.cc
 )

# 编译生成静态库(or动态库)
add_library(mudlite_base ${base_SRCS})

# 让mudlite库链接其它库
# target_link_libraries(none)

install(TARGETS mudlite_base DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/mudlite/base)