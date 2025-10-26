cmake_minimum_required(VERSION 3.22.1)
project(mudlite)

# 将boost库模块去除、完全使用c++语言级
# 设置调试信息 以及启动C++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++17 -fPIC")

# 最终编译成so动态库，设置动态库的路径，放在根目录下lib文件夹下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 定义参与编译的源代码文件
aux_source_directory(./mudlite/base         SRC_LIST)
aux_source_directory(./mudlite/net          SRC_LIST)
aux_source_directory(./mudlite/net/poller   SRC_LIST)
# 编译生成动态库mudlite
add_library(mudlite SHARED ${SRC_LIST})

add_subdirectory(./mudlite/base) 
add_subdirectory(./mudlite/net)