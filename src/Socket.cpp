#include "Socket.h"
//C++ 老手们习惯在调用系统级 API（如 socket, bind, listen, setsockopt, close）时，统统加上 ::。
int createnonblocking() {
	int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
	//如果 listenfd 是阻塞的： 当你循环处理完最后第 10 个连接，试图去 accept 第 11 个（此时队列已空）时，你的线程就会卡死在第 11 次 accept 上，再也回不去 epoll_wait 了。

	//如果 listenfd 是非阻塞的： 第 11 次 accept 会返回 EAGAIN，你就可以优雅地跳出循环。
	if (listenfd < 0) {
		perror("socket() failed"); exit(-1);
	}
	return listenfd;
}

Socket::Socket(int fd) :fd_(fd){

}
Socket::~Socket() {
	::close(fd_);
}

int Socket::fd() const {
	return fd_;
}

std::string Socket::ip() const {
	return ip_;
}
uint16_t Socket::port() const {
	return port_;
}
void Socket::setreuseaddr(bool on) {
	int optval = on ? 1 : 0;
	::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
}
void Socket::setreuseport(bool on) {
	int optval = on ? 1 : 0;
	::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
}
void Socket::settcpnodelay(bool on) {
	int optval = on ? 1 : 0;
	::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
}
void Socket::setkeepalive(bool on) {
	int optval = on ? 1 : 0;
	::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::set_ip_port(const std::string& ip, uint16_t port) {
	ip_ = ip;
	port_ = port;
	return;
}
void Socket::bind(const InetAddress& servaddr) {
	if (::bind(fd_, servaddr.addr(), sizeof(sockaddr_in)) < 0) {
		perror("bind() failed"); close(fd_); exit(-1);
	}
	ip_ = servaddr.ip();
	port_ = servaddr.port();
}
//编译的时候自动把它变成了 servsock.listen(128);  所以不加 int n = 128
void Socket::listen(int n) {
	if (::listen(fd_, n) != 0) {
		perror("listen() failed"); close(fd_); exit(-1);
	}
}

int Socket::accept(InetAddress& clientaddr) {
	struct sockaddr_in peeraddr {};     //要把peeraddr 传给闯进来的 结构体   这个结构类得有赋值函数
	socklen_t len = sizeof(peeraddr);
	int clientfd = accept4(fd_, (struct sockaddr*)(&peeraddr), &len, SOCK_NONBLOCK);
	//你创建的是一个“局部新对象”，而不是修改“参数里的那个对象”。
	//InetAddress clientaddr(peeraddr);
	clientaddr.setaddr(peeraddr);
	//在socket类中只有服务端才调用accept 
	//ip_ = clientaddr.ip();   这是把客服端的地址赋给了服务器啊  servsock_->accept(clientaddr) 在acceptor运行的是这个
	//port_ = clientaddr.port();
	//这么做只能改变服务器的ip 和port

	return clientfd;
}// <--- 2. 函数结束，这个局部的 clientaddr 被销毁了！
  // 3. 外面的那个 clientaddr 依然是空的，啥也没得到。