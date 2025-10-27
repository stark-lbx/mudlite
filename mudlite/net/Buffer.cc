#include "Buffer.h"
#include "../base/logger.h"

#include <errno.h>
#include <string.h>
#include <sys/uio.h> // for readv, writev
#include <unistd.h> // for write
#include <sys/types.h>

using namespace mudlite;
using namespace mudlite::net;

// Buffer缓冲区是有大小的, 但是从fd上度数据的时候, 却不知道tcp数据最终的大小
ssize_t Buffer::readFd(int fd, int *savedErrno)
{
    char extrabuf[65536] = {0}; // 栈内存空间: 64k
    struct iovec vec[2];
    // readv : 将读到的数据放到不连续的一些内存中
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    // 每次读取上限为: 64k, 不够64k, extrabuf补充
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if(n < 0)
    {
        *savedErrno = errno;
        // LOG_FATAL("%d: %s", errno, ::strerror(errno));
    }
    else if((size_t)n <= writable)
    {
        // 直接扩展即可、未扩容
        writerIndex_ += n;
    }
    else{
        // 已经填充好的更新一下位置先
        writerIndex_ = buffer_.size();
        // 从extrabuf中将补充的数据追加到自己这个buffer中
        append(extrabuf, n-writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* savedErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0)
    {
        *savedErrno = errno;
    }
    return n;
}
