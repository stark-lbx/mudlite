// clang-format off
#pragma once

/**
 * 用户使用mudlite库编写服务器程序
 *  直接使用对外提供的最简接口：TcpServer
 */

#include "../base/noncopyable.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddr.h"
#include "TcpConnection.h"

#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>

namespace mudlite
{
namespace net
{

// 对外编程使用的类 

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort
    };
    
    ~TcpServer();
    TcpServer(EventLoop* loop, 
              const InetAddr &listenAddr, 
              const std::string& nameArg, 
              Option option = kNoReusePort);

    void setThreadInitCallback(const ThreadInitCallback &cb) {threadInitCallback_ = cb;}
    void setConnectionCallback(const ConnectionCallback &cb) {connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback &cb) {messageCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {writeCompleteCallback_ = cb;}

    // 设置底层subloop的个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();

    const std::string& ipPort() const { return ipPort_;}
    const std::string& name() const {return name_;}
    EventLoop* getLoop() const {return loop_;}


private:
    void newConnection(int sockfd, const InetAddr& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* loop_; // baseLoop : 用户定义的loop
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_; // 运行在mainloop，任务就是监听新连接事件
    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread

    ConnectionCallback connectionCallback_; //有新连接到来时的回调
    MessageCallback messageCallback_; // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息返送完毕之后的回调
    ThreadInitCallback threadInitCallback_; // loop线程初始化时的回调

    std::atomic_int started_; // server是否在运行
    int nextConnId_;
    ConnectionMap connections_; // 连接映射表
};

} // net
} // mudlite