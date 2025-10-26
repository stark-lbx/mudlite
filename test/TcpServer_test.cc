#include <mudlite/net/EventLoop.h>
#include <mudlite/net/TcpServer.h>

int main()
{
    // 创建mainLoop
    mudlite::net::EventLoop loop;
    mudlite::net::TcpServer server(&loop,
                                   mudlite::net::InetAddr(7868, "127.0.0.1"),
                                   "EchoServer");

    server.setConnectionCallback(
        [](const mudlite::net::TcpConnectionPtr &conn)
        {
            LOG_INFO("EchoServer - %s -> %s is %s",
                     conn->peerAddress().toIpPort().c_str(),
                     conn->localAddress().toIpPort().c_str(),
                     (conn->connected() ? "UP" : "DOWN"));
        });
    server.setMessageCallback(
        [](const mudlite::net::TcpConnectionPtr &conn, mudlite::net::Buffer *buf, mudlite::Timestamp time)
        {
            std::string msg(buf->retrieveAllAsString());
            conn->send(msg);
        });


    server.setThreadNum(5);

    server.start();
    loop.loop();

    return 0;
}
