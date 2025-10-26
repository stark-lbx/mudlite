// clang-format off
#pragma once

#include <vector>
#include <string>
#include <algorithm>

namespace mudlite
{
namespace net
{

/**
 * buffer model:
 * /// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
 * ///
 * /// @code
 * /// +-------------------+------------------+------------------+
 * /// | prependable bytes |  readable bytes  |  writable bytes  |
 * /// |                   |     (CONTENT)    |                  |
 * /// +-------------------+------------------+------------------+
 * /// |                   |                  |                  |
 * /// 0      <=      readerIndex   <=   writerIndex    <=     size
 * /// @endcode
 * prependable bytes: 预留字节, 
 */

class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend) {}

    size_t readableBytes() const {return writerIndex_ - readerIndex_;}
    size_t writableBytes() const {return buffer_.size() - writerIndex_;}
    size_t prependableBytes() const {return readerIndex_;}

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const 
    {return begin() + readerIndex_;}

    // onMessage: string <- buffer
    // retrieve: 中文释义: 找回、检索、取回. 应该是回收的意思
    void retrieve(size_t len)
    {
        // 剩余可消费数据 比 想要消费的数据 多才行
        if(len <= readableBytes())
        {
            readerIndex_ += len;
        }
        else 
        {
            // 不然消费完即可, 不一定严格达到len
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        /**
         * state1:
         *    预留        可读                   可写
         * |--------|abcdefghijklmnfkajlsfsdsdf|->
         *          ↓                          ↓
         *        r_index                     w_index
         * 第一个状态: 收到数据但都未处理
         * 
         * state2:
         *    预留       可读(null为已消费)       可写
         * |--------|     fghijklmnfkajlsfsdsdf|->
         *                ↓                    ↓
         *              r_index               w_index
         * 第二个状态: 消费了一部分数据, 还剩余一些没处理
         * 
         * state3:
         *    预留       可读                    可写
         * |--------|                          |->
         * 这是一个中间状态，全部数据被读完了, 那么, 如果将r_index指向此时的w_index, 那么之前完全消费过的位置就浪费了
         * 所以, retrieveAll的效果就是 r_index == w_index
         * 既然如此，为了减少继续扩容的开销, 那么直接将r_index 与 w_index 都置为 begin() + kCheapPrepend即可
         * 
         */
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    // 把onMessage函数上报的buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }


    // 扩容函数
    void makeSpace(size_t len)
    {
        // at end of function, there is a graphics draw by stark

        // 后面可写的容量 加上 前面已经标记消费了的(readableIndex_ - kCheapPrepend)
        // 利用起来还不足以 满足 len 个长度, 那么就是真的需要扩容
        if(writableBytes() + prependableBytes() - kCheapPrepend < len)
        {
            // 扩充到正好的位置（stark-notes: 可能会导致频繁扩容）
            // 但实际上可能随着服务端对客户端消息的迅速消费，可能并不会如此
            buffer_.resize(writerIndex_ + len);
        }
        else // 此时可以考虑将之前的空间利用起来
        {
            // 也就是将剩余的可消费数据移动到开头(kCheapPrepend紧后跟着的一个位置)
            std::copy(begin() + readerIndex_, // src_begin 
                      begin() + writerIndex_, // src_end
                      begin() + kCheapPrepend); // dst_head
            // copy必须是顺序拷贝，所以才选的这个算法
            // 如果自己写一个copy，从后往前copy可能会导致源数据被覆盖, 所以此处必须从前往后顺序copy
            // 如果自己写的copy 是从前向后拷贝就没有任何问题，这里只是作为提示出现
        }

        // assume: 
        //      '0': retrieved 
        //      '1': no-retrieved
        //      '.': no-use
        //
        // initState:
        // |--------|000000111111|....|
        //                 ↑     ↑    ↑
        //               r_idx  w_idx  end
        // now: 
        //      prependBytes = 8
        //      retrievedBytes = 6
        //      no-retrievedBytes = 6
        //      capacity = 8+12+4 = 24 Bytes
        // want to makeSpace(6)
        // writableBytes == 4, less than 6(need)
        // but, retrieved_length + writableBytes == 10 > 6(need)
        // then， memcopy->reuslt:
        //
        // afterState:
        // |--------|111111|..........|
        //          ↑      ↑          ↑
        //          r_idx  w_idx      end
    
    }

    // 保证可写len个字节(扩容 or pass)
    void ensureWritableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            // 可写的字节不足 len 
            makeSpace(len); // 创建len个空间
        }
    }

    void append(const char* /*restrict*/data, size_t len)
    {
        // 1.确保len长度可写（避免消息丢失）
        ensureWritableBytes(len);
        // 2.拷贝数据到缓冲区
        std::copy(data, data+len, beginWrite());
        // 3.修改可写指针位置
        writerIndex_ += len;
    }

    char* beginWrite()
    {return begin() + writerIndex_;}
    const char* beginWrite() const
    {return begin() + writerIndex_;}

    // 从 fd 上读取数据
    ssize_t readFd(int fd, int* savedErrno);
    // send
    ssize_t writeFd(int fd, int* savedErrno);
private:
    char* begin() 
    {return &*buffer_.begin();}

    const char* begin() const 
    {return &*buffer_.begin();}

    

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};

} // namespace net
} // namespace mudlite