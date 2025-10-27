# mudlite
`**（参考开源库：muduo；学习使用、如侵删）**`
> mudlite是一个基于Reactor模型的轻量级C++网络库，项目借鉴了muduo库的设计思想，剥离了Boost依赖，完全基于C++17及以下标准。  
> 完成了：事件循环、连接管理、IO复用、缓冲区等核心组件。【异步日志系统、定时器模块暂未实现】  
> 支持高并发网络通信场景，可以快速搭建各种网络服务。

## Reactor模型
网络IO模型：
- 根据 数据准备阶段 是否等待 判断
    - 阻塞IO
    - 非阻塞IO
- 根据 数据拷贝阶段 是否等待 判断
    - 同步IO
    - 异步IO
- 此外
    - IO多路复用
    - 信号驱动IO
> 虽然这里写的是六个，但实际上数据准备和数据拷贝经常组合到一起讨论  
> - 1.同步阻塞IO：read(fd); 读缓冲区没有数据，等着缓冲区有数据（阻塞）；有了之后在这等着从内核拷贝到用户态的buf中，拷贝完才返回（同步）；  
> - 2.同步非阻塞IO：read(nonblock_fd); 读缓冲区没有数据，直接返回并设置errno，用户态先去忙别的，等下再调用read（非阻塞）；如果有一次调用有数据了，等着拷贝完就返回了
> - 3.异步非阻塞IO：重点说异步的概念：就是，当有数据时，内核完成拷贝，在这个拷贝期间，用户态可以执行其它任务，后面内核完成之后会通知内核拷贝完了，直接就用这个buf就可ok
> - 4.信号驱动IO：Linux的信号通信想必是熟悉的；信号驱动IO是指，在数据准备阶段，用户不必每次都去调用read去询问好了没（每次调用也都是一次开销），数据准备好了之后，内核自会通知；（好像和异步有点像，但是，数据拷贝阶段是同步的，是一种特殊的同步非阻塞IO模型）
> - 5.IO多路复用：上述的IO都是单个fd的解释；而IO多路复用解决的是监听多个文件描述符，让这个检测文件描述符是否有读写事件的过程托管给内核，内核检测到有事件，返回给用户。（避免用户态维护多个fd、每次都去内核问这个好了没，那个好了没）

> Reactor（反应堆）模型就是基于IO多路复用，将IO操作当作一个个事件（实际上也确实是读写事件）。  
> 事件驱动模式，允许一个或多个输入同时传递给服务器处理器，服务端程序处理传入的多路请求，并将它们同步分派给请求对应的处理线程。  

Rector的核心组件就是：
- Event（事件源）
- EventHandler（事件处理器）
- Reactor（反应器）
- EventDemultiplexer（事件分发器）
> 其中：事件源就是产生事件的源头；在Linux中，我们可以通过 fd 寻找到事件源头  
>       事件处理器，就是当事件发生后，对这个事件做的一个操作
>       事件分发器，就是一个系统调用，像epoll_wait、select、poll等，感知事件源上的事件发生
>       反应器，就是一个循环，在循环过程中不断的通过 事件分发器 拿到 对应的就绪事件，然后执行相应的 Handler 操作


## muduo库 - ChenShuo
muduo库是陈硕大神用C++写的一个网络库，性能很好，也很普适。  
这个网络库的框架是 one loop per thread：即每个线程都有一个loop。  
通常采用单线程循环，即只有一个EventLoop来处理连接和事件。因此默认情况下，muduo库不会使用多个线程来同时监听同一个套接字的accept事件、从而避免了惊群效应  
但是也支持多线程模式，即使用多个EventLoop（每个EventLoop运行在一个独立的线程中）来处理事件。
在这种情况下，muduo库采用：
    一个MainReactor：即BaseLoop，运行accept接收所有的fd及其事件，不干其他的事。
    多个SubReactor：即IOLoop，运行poll监听管理的fd列表的事件，然后调用读写操作。
    因为多个SubReactor的工作是一样的（且每个线程都是运行在同一台主机上、同一个进程内，地位、资源等限制都是一致的），所以，这里默认采用轮询来做负载均衡。


## 个人学习理解
1、根据个人理解，muduo库的Reactor模型的简易的关系图是这样的：
```cpp
Channel  ↘
Channel -（Poller） ->-> EventLoop -> EventLoopThread
Channel  ↗                                  ↓
                                    EventLoopThreadPool  <--has-- DiyServer
Channel  ↘                                  ↑
Channel -（Poller） ->-> EventLoop -> EventLoopThread
Channel  ↗ 

(
    一个loop内并没有保存多个Channel，而是有一个ChannelBuffer，实际保存多个Channel的是Poller，EventLoop中有一个Poller对象
    而 EventLoop 只负责将 poll 到 ChannelBuffer 中的那些就绪的 Channels 进行回调执行
)

(EvetnLoopThread::start() -> EventLoop::loop() -> Poller::poll() -> epoll_wait() 操作)
```

2、然后muduo库主要做了 TcpServer 运行在底层，其他的上层服务器均可在TcpServer的基础上做扩展
TcpServer 涉及到的一些类：
- InetAddr：对 sockaddr_in 的封装，提供了获取 ip 和 port 的接口
- Socket：对 socket 的封装，提供了 bind、listen、accept 等接口
- Buffer：设计了一个缓冲区（字符数组），用于存放接收到的数据，每个连接都会有一个输入缓冲区和输出缓冲区（解决Tcp粘包半包问题）

- Acceptor：其实就是一个特殊事件处理器（EventHandler），内部有一个事件（在这里将事件抽象为了 Channel 而不是 Event）。
> 这个类里的Channel是负责监听的，也就是socket创建的sockfd，每个服务器只有这么一个sockfd。
- TcpConnection：也是一个事件处理器（EventHandler），内部有一个事件（Channel）
> 这个类的Channel里面的事件不是读就是写，这里的读写操作需要外部服务器自行编写处理逻辑（根据业务），这里只做回调。
> TcpConnection在Server中有很多。而这么多的TcpConnection里面的那一个Channel就被负载均衡到了某一个Loop中

```cpp
                                                             --分配一个连接--> (SubReactor::loop) -> (register :->: onMessage())
(MainReactor::loop) -> (Acceptor::accept)-> (TcpConnections) --分配一个连接--> (SubReactor::loop) -> (register :->: onMessage())
                                                             --分配一个连接--> (SubReactor::loop) -> (register :->: onMessage())
↑__________________________________________________________↑
                                ↓
                            Server主体
所以MainReactor的loop操作通过epoll_wait得到了连接事件，调用这个 Channel->handleEvent() 就 让Acceptor去执行accept操作
而SubReactor，通过loop操作，使用epoll_wait得到的事件全是读写事件，调用 Channel->handleEvent() 就是 执行注册的读写回调

Server主体部分还用一个poll来监听这个accpet事件，有点大财小用？如果服务器同时监听多个端口，也就是创建了多个sockfd，那么就有用了
(好像也不这么用吧，我在这块给我自己遗留一个问题)
```