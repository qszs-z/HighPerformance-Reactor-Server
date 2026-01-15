#include "Channel.h"
#include "EventLoop.h"   // 要调用 loop_->updatechannel()
#include "Socket.h"      // 要调用 servsock->accept()
#include "InetAddress.h" // 要创建 InetAddress clientaddr
#include <unistd.h>      // close, read, write
#include <iostream>      // printf, cout
#include "Connection.h"
#include <memory>
Channel::Channel(EventLoop* loop, int fd)
:loop_(loop),fd_(fd)
{

}

Channel::~Channel()
{
	//不要销毁ep  关闭fd  不属于他
}
int Channel::fd() {
	return fd_;
}

void Channel::useet() {
	events_ = events_ | EPOLLET;
}

void Channel::enablereading() {  //向 epoll 内核红黑树【注册】读事件
	events_ = events_|EPOLLIN;
	//初始状态（假设你调用过 useet()）： events_ = 1000 0000 (只开了边缘触发)
	//你想加读事件(EPOLLIN = 0000 0001)： events_ | EPOLLIN
	//保留你原本的设置，并在此基础上，把‘读’的开关也打开。
	loop_->updatechannel(this);
}
void Channel::disablereading()
{
	events_ &= ~EPOLLIN;
	loop_->updatechannel(this);
}
void Channel::enablewriting()
{
	events_ |= EPOLLOUT;
	loop_->updatechannel(this);
}
void Channel::disablewriting()
{
	events_ &= ~EPOLLOUT;
	loop_->updatechannel(this);
}
void Channel::disableall()       //取消全部的事件    当tcp断开是
{
	events_ = 0;
	loop_->updatechannel(this);
}
	
void Channel::remove()         //从事件循环中删除channel。
{
	//disableall();       //可以不写
	loop_->removechannel(this);           //调用eventloop中的这个函数。从红黑树删除fd
}
void Channel::setinepoll() {
	inepoll_ = true;
}
void Channel::setrevents(uint32_t ev) {
	revents_ = ev;
}
bool Channel::inepoll() {
	return inepoll_;
}
uint32_t Channel::events() {
	return events_;
}
uint32_t Channel::revents() {
	return revents_;
}

void Channel::handleevent()
{
	//直接使用channel类里面的成员变量
	if (revents_ & EPOLLRDHUP) {    
		std::cout << "EPOLLRDHUP" << std::endl;
		// 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
		//printf("client(event_fd=%d) disconnected.\n", fd_);
		//close(fd_);            // 关闭客户端的fd。
		//remove();         //如果断开了删除channel
		closecallback_();     // 回调Connection::closecallback()。     remove(）已在回调函数中
		//clientchannel_->setclosecallback(std::bind(&Connection::closecallback, this));
		//closecallback_();调用的是 Connection::closecallback
		//void Connection::setclosecallback(std::function<void(Connection*)> fn) {
		//closecallback_ = fn;
		//conn->setclosecallback(std::bind(&TcpServer::closeconnection, this,std::placeholders::_1));
		//Connection::closecallback 调用的是 
		//void TcpServer::closeconnection(Connection* conn)    要一个参数  这个参数是Connection所以要在
		//第二个回调设置参数
	}
	//读事件
	else if (revents_ & (EPOLLIN | EPOLLPRI)) // 接收缓冲区中有数据可以读。
	{
		//std::cout << "EPOLLIN | EPOLLPRI" << std::endl;
		readcallback_();
	}
	//这里没法发送connction的发送缓冲区因为这个在connect类里面调用回调函数
	else if (revents_ & EPOLLOUT) {   // 有数据需要写，暂时没有代码，以后再说。
		//std::cout << "EPOLLOUT" << std::endl;
		writecallback_();
	}
	else {                          // 其它事件，都视为错误。
		//printf("client(event_fd=%d) error.\n", fd_);
		//close(fd_);            // 关闭客户端的fd。
		//remove();         //如果断开了删除channel
		errorcallback_();     // 回调Connection::errorcallback()。  remove(）已在回调函数中
	}
}
/*
void Channel::newconnection(Socket* servsock)
{
	//struct sockaddr_in peeraddr;
	// socklen_t len = sizeof(peeraddr);
	//int clientfd = accept4(listenfd, (struct sockaddr*)(&peeraddr), &len, SOCK_NONBLOCK);
	//设置非阻塞的原因read/write 时不卡死。
	InetAddress clientaddr; //缺省的构造函数
	std::unique_ptr<Socket> clientsock = std::make_unique<Socket>(servsock->accept(clientaddr));   
	//把所有全交给 Connection 让他释放去
	std::cout << "accept client(fd=" << clientsock->fd()
		<< ",ip=" << clientaddr.ip()   // 这里的 ip() 返回 std::string 也没问题！
		<< ",port=" << clientaddr.port()
		<< ") ok." << std::endl;
	//要把所有权都交出去是不是就是在传给的哪个类里面释放了
	//std::move()  交出所有权
	//把一个 左值 强制转换为 右值引用 (T&&)std::move(clientsock)
	Connection* conn = new Connection(loop_, std::move(clientsock));  //这里也没有释放以后解决
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

}
*/
//void Channel::onmessage()
//{
//	char buf[1024];
//	while (true) {
//		memset(buf, 0, sizeof(buf));
//		ssize_t n = read(fd_, buf, sizeof(buf));
//		if (n > 0) {
//			// 把接收到的报文内容原封不动的发回去。
//			printf("recv(event_fd=%d):%s\n", fd_, buf);
//			send(fd_, buf, n, 0);
//		}
//		else if (n == -1 && errno == EINTR) {// 读取数据的时候被信号中断，继续读取。
//			continue;
//		}
//		else if (n == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) { // 全部的数据已读取完毕。
//			break;
//		}
//		else if (n == 0) {// 客户端连接已断开。
//			/*	printf("client(event_fd=%d) disconnected.\n", fd_);
//				close(fd_);*/
//			closecallback_();
//			//clientchannel_->setclosecallback(std::bind(&Connection::closecallback, this));
//			//closecallback_();调用的是 Connection::closecallback
//			//
//			break;
//		}
//	}
//}

void Channel::setreadcallback(std::function<void()> fn)
{
	readcallback_ = fn;
	//std::bind 的核心作用：“提前传参”
}
void Channel::setclosecallback(std::function<void()>fn) {
	closecallback_ = fn;
}
void Channel::seterrorcallback(std::function<void()>fn) {
	errorcallback_ = fn;
}

void Channel::setwritecallback(std::function<void()> fn)
{
	writecallback_ = fn;
}
