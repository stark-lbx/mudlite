#include <mudlite/net/EventLoop.h>
#include <mudlite/net/TcpServer.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
    // 将输出文件描述符冲顶向到一个log文件
    // int fd = ::open("./log.txt", O_RDWR | O_TRUNC);
    // ::dup2(fd, STDOUT_FILENO);

    // 创建mainLoop
    mudlite::net::EventLoop loop;
    mudlite::net::TcpServer server(&loop,
                                   mudlite::net::InetAddr(5678, "0.0.0.0"),
                                   "EchoServer");

    server.setConnectionCallback(
        [](const mudlite::net::TcpConnectionPtr &conn)
        {
            LOG_INFO("EchoServer - %s -> %s is %s",
                     conn->peerAddress().toIpPort().c_str(),
                     conn->localAddress().toIpPort().c_str(),
                     (conn->connected() ? "UP" : "DOWN"));
            if(!conn->connected()){
                conn->shutdown();
            }
        });
    server.setMessageCallback(
        [](const mudlite::net::TcpConnectionPtr &conn, mudlite::net::Buffer *buf, mudlite::Timestamp time)
        {
            std::string msg(buf->retrieveAllAsString());

            std::string header = "HTTP/1.1 200 OK\r\n";
            std::string Content_type = "Content-Type: text/plain\r\n";
            std::string content_length = "Content-Length: " + std::to_string(msg.length()) + "\r\n";
            msg = header + Content_type + content_length + "\r\n" + msg + "\r\n";

            conn->send(msg);
        });


    // server.setThreadNum(5);

    server.start();
    loop.loop();

    return 0;
}
