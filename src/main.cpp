//#include <iostream>
//#include <memory>
//#include <cstdio>    
//#include <unistd.h>  
//#include <cstdlib>   
//#include <cstring>   
//#include <cerrno>   
//#include <sys/socket.h>
//#include <sys/types.h>          // 涉及代码：ssize_t, size_t, pid_t, socklen_t
//#include <arpa/inet.h>    // 涉及代码：htons(主机转网络序), ntohs, inet_pton(IP转二进制), inet_ntop(二进制转IP), ine
//#include <sys/fcntl.h>     // 涉及代码：fcntl, O_NONBLOCK (非阻塞标志)
//#include <sys/epoll.h>
//#include <netinet/tcp.h>      // TCP_NODELAY需要包含这个头文件。
//#include "InetAddress.h"
//#include "Socket.h"
//#include "Epoll.h"
//#include "Channel.h"
//#include "EventLoop.h"
#include "EchoServer.h"
#include <csignal>
//class Channel {
//private:
//    int fd_;
//    bool islisten = false; //true 表示监听的fd   false 客户端连上的fd
//    //可以声明更多的成员
//public:
//
//    Channel(int fd,bool islisten = false) :fd_(fd),islisten(islisten){};
//    int fd() { return fd_ };
//    bool islisten() { return islisten };
//};
EchoServer* echoserver;
void Stop(int sig) {    //信号2 和信号15的处理函数，功能是停止服务程序。
    printf("sig= %d\n", sig);
    //调用Echoserver::Stop()停止服务
    echoserver->Stop();
	printf("echoserver已停止。\n");
	delete echoserver; // 触发析构函数
    //IO 线程和工作线程还在店里干活，或者坐在那里发呆（阻塞在 epoll_wait）。不会执行delete echoserver的后面
    //要先删除了IO 线程和工作线程  现在已经写好了使用echoserver->Stop();
    printf("delete echoserver。\n");
    exit(0);
}

//int main(int argc,char * argv[])
//{
//    if (argc != 3) {
//        std::cout << "usage: ./TcpServer ip port\n";
//        std::cout << "example: ./TcpServer 127.0.0.1 8152\n\n";
//        return -1;
//    }
//    //Socket servsock(createnonblocking());  //创建socket
//    //InetAddress servaddr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
//    //servsock.setreuseaddr(true);
//    //servsock.setkeepalive(true);
//    //servsock.setreuseport(true);
//    //servsock.settcpnodelay(true);
//    //servsock.bind(servaddr);
//    //servsock.listen();
//    /*
//    int epollfd = epoll_create(1);              // 创建epoll句柄（红黑树）。
//    Channel * servchannel = new Channel(listenfd,true);
//    struct epoll_event ev;           // 声明事件的数据结构。
//    ev.data.ptr = servchannel;
//    ev.data.fd = servsock.fd();         // 指定事件的自定义数据，会随着epoll_wait()返回的事件一并返回。
//    ev.events = EPOLLIN;          // 让epoll监视listenfd的读事件，采用水平触发。
//
//    epoll_ctl(epollfd, EPOLL_CTL_ADD, servsock.fd(), &ev);
//    struct epoll_event evs[10];
//    */
//    ////Epoll ep;
//    //EventLoop loop;
//    //Channel* servchannel = new Channel(loop.ep(), servsock.fd());
//    ////这里设置了回调函数  使用readcallback是会调用 Channel::newconnection,
//    //servchannel->setreadcallback(std::bind(&Channel::newconnection, servchannel,&servsock));
//    //servchannel->enablereading();
//    //TcpServer tcpserver(argv[1], static_cast<uint16_t>(atoi(argv[2])));
//    //tcpserver.start();   //运行事件循环。
//    //ep.addfd(servsock.fd(), EPOLLIN);
//    //std::vector<epoll_event> evs;
//    /*
//    while (true) {
//        std::vector<Channel*>channels = ep.loop();
//        /*
//        // 把有事件发生的传到 ev_s 中
//        int infds = epoll_wait(epollfd, evs, 10, -1);
//        //有数据发过来，内核把进程叫醒，并把发生的事件清单填到 evs 数组里，返回事件数量 infds。
//        //返回失败
//        if (infds < 0) {
//            perror("epoll_wait() failed"); break;
//        }
//		// 超时。
//		if (infds == 0)
//		{
//			printf("epoll_wait() timeout.\n"); continue;
//		}
//        // 如果infds>0，表示有事件发生的fd的数量。
//        */
//    /*
//       // evs = ep.loop();
//        //for (int i = 0; i < infds ; i++) {
//        //下面这些代码之和channel有关系
//        for (auto& ch : channels) {
//            ch->handleevent();
//            /*
//            if (ch->revents() & EPOLLRDHUP) {    // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
//                printf("client(event_fd=%d) disconnected.\n", ch->fd());
//                close(ch->fd());            // 关闭客户端的fd。
//            }
//            else if (ch->revents() & (EPOLLIN | EPOLLPRI)) // 接收缓冲区中有数据可以读。
//            {
//                //Channel * ch =(Channel *) evs[i].data.ptr
//                //if(ch->islisten() == true)   //然后把后面的都改成 ch->fd()
//                if (ch == servchannel) {  // servchannel里面有servsock.fd()
//                    //struct sockaddr_in peeraddr;
//                   // socklen_t len = sizeof(peeraddr);
//                    //int clientfd = accept4(listenfd, (struct sockaddr*)(&peeraddr), &len, SOCK_NONBLOCK);
//                    //设置非阻塞的原因read/write 时不卡死。
//                    InetAddress clientaddr; //缺省的构造函数
//                    Socket* clientsock = new Socket(servsock.accept(clientaddr));
//
//                    std::cout << "accept client(fd=" << clientsock->fd()
//                        << ",ip=" << clientaddr.ip()   // 这里的 ip() 返回 std::string 也没问题！
//                        << ",port=" << clientaddr.port()
//                        << ") ok." << std::endl;
//                    //channel * clientchannel = new Channel(clientsock->fd());
//                    // ev.data.ptr= clientchannel;
//                    //ev.data.fd = clientsock->fd();
//                    //ev.events = EPOLLIN | EPOLLET;
//                    //epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsock->fd(), &ev);
//                    Channel* clientchannel = new Channel(&ep, clientsock->fd(),false);
//                    clientchannel->useet();
//                    clientchannel->enablereading(); //enablereading 里有 updatechannel 可以添加事件
//                    //ep.addfd(clientsock->fd(), EPOLLIN | EPOLLET);
//                }
//                else {
//                    char buf[1024];
//                    while (true) {
//                        memset(buf, 0, sizeof(buf));
//                        ssize_t n = read(ch->fd(), buf, sizeof(buf));
//                        if (n > 0) {
//                            // 把接收到的报文内容原封不动的发回去。
//                            printf("recv(event_fd=%d):%s\n", ch->fd(), buf);
//                            send(ch->fd(), buf, n, 0);
//                        }
//                        else if (n == -1 && errno == EINTR) {// 读取数据的时候被信号中断，继续读取。
//                            continue;
//                        }
//                        else if(n == -1 && ((errno == EAGAIN)||(errno == EWOULDBLOCK))){ // 全部的数据已读取完毕。
//                            break;
//                        }
//                        else if (n == 0) {// 客户端连接已断开。
//                            printf("client(event_fd=%d) disconnected.\n", ch->fd());
//                            close(ch->fd());
//                            break;
//                        }
//                    }
//
//                }
//            }
//            else if (ch->revents() & EPOLLOUT) {   // 有数据需要写，暂时没有代码，以后再说。
//
//            }
//            else {                          // 其它事件，都视为错误。
//                printf("client(event_fd=%d) error.\n", ch->fd());
//                close(ch->fd());            // 关闭客户端的fd。
//            }
//        }
//        
//        }
//    }
//    */
//
//    signal(SIGTERM, Stop);    // 信号15，系统kill或killall命令默认发送的信号。
//    signal(SIGINT, Stop);        // 信号2，按Ctrl+C发送的信号。
//    //几乎没有计算工作写0
//    echoserver=new EchoServer(argv[1], static_cast<uint16_t>(atoi(argv[2])),5,0,1);//io写3 工作写0  第三个参数是报文格式
//    echoserver->Start();
//
//    return 0;
//}