#include "EventLoop.h"
#include "Acceptor.h"
#include <memory>
#include <functional>
#include "Connection.h"
#include <iostream>
Acceptor::Acceptor(EventLoop* loop, const std::string& ip, const uint16_t port)
	:loop_(loop),servsock_(createnonblocking()),acceptchannel_(loop_, servsock_.fd())
{
	// 1. 创建 Socket (这也是老师逻辑，只是换成了智能指针)
	// createnonblocking() 假设是你的全局函数或 Socket 静态函数
	// 如果是全局函数直接调用，如果是静态函数用 Socket::createnonblocking()
	//servsock_ =std::make_unique<Socket>(createnonblocking());  //创建socket    没释放
	InetAddress servaddr(ip, port);
	servsock_.setreuseaddr(true);
	servsock_.setkeepalive(true);
	servsock_.setreuseport(true);
	servsock_.settcpnodelay(true);
	servsock_.bind(servaddr);
	servsock_.listen();
	//acceptchannel_ = std::make_unique<Channel>(loop_, servsock_.fd());  // 没释放
	//这里设置了回调函数  使用readcallback是会调用 Channel::newconnection,
	acceptchannel_.setreadcallback(std::bind(&Acceptor::newconnection, this));
	//std::bind 会尝试拷贝参数。unique_ptr 是独占的，禁止拷贝
	//把智能指针里的 裸指针 (Raw Pointer) 拿出来传进去，使用 .get() 方法。
	acceptchannel_.enablereading();
}

Acceptor::~Acceptor()
{
	//使用了unique智能指针 不需要手动释放
	//delete servsock_;
	//delete acceptchannel_;
	//注意loop_是从外面传进来的不要释放
	//现在改成站=栈对象了。
}
void Acceptor::newconnection()
{
	//struct sockaddr_in peeraddr;
	// socklen_t len = sizeof(peeraddr);
	//int clientfd = accept4(listenfd, (struct sockaddr*)(&peeraddr), &len, SOCK_NONBLOCK);
	//设置非阻塞的原因read/write 时不卡死。
	InetAddress clientaddr; //缺省的构造函数
	std::unique_ptr<Socket> clientsock = std::make_unique<Socket>(servsock_.accept(clientaddr));
	//把所有全交给 Connection 让他释放去
	clientsock->set_ip_port(clientaddr.ip(), clientaddr.port());

	//std::cout << "accept client(fd=" << clientsock->fd()
	//	<< ",ip=" << clientsock->ip()   // 这里的 ip() 返回 std::string 也没问题！
	//	<< ",port=" << clientsock->port()
	//	<< ") ok." << std::endl;
	//要把所有权都交出去是不是就是在传给的哪个类里面释放了
	//std::move()  交出所有权
	//把一个 左值 强制转换为 右值引用 (T&&)std::move(clientsock)
	// //下面放到 TCPServer中了
	//Connection* conn = new Connection(loop_, std::move(clientsock));  //这里也没有释放以后解决
	//unique_ptr 只能通过右值（移动构造）来传递。
	//Connection 对象诞生后，它就独占了这个 Socket。
	// 外界（比如 Server 或 Acceptor）不再需要控制这个 Socket 了，它的生命周期将完全绑定在 Connection 身上。
	//当你把 unique_ptr 移动（move）进 Connection 类成员变量后，Connection 就背上了“锅”。
	//生： Connection 活着，Socket 就活着。
	//死： 当 Connection 被析构（delete 或智能指针引用计数归零）时，Connection 的成员变量 std::unique_ptr<Socket> socket_ 会自动调用析构函数。

	//channel * clientchannel = new Channel(clientsock->fd());
	// ev.data.ptr= clientchannel;
	//ev.data.fd = clientsock->fd();
	//ev.events = EPOLLIN | EPOLLET;
	//epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsock->fd(), &ev);
	//Channel* clientchannel = new Channel(loop_, clientsock->fd());   //没有释放
	//clientchannel->setreadcallback(std::bind(&Channel::onmessage, clientchannel));
	//clientchannel->useet();
	//clientchannel->enablereading(); //enablereading 里有 updatechannel 可以添加事件
	////ep.addfd(clientsock->fd(), EPOLLIN | EPOLLET);
	newconnectioncb_(std::move(clientsock));
}

void Acceptor::setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn)
{
	newconnectioncb_ = fn;
}
