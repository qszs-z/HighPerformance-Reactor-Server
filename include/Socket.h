#pragma once
#include <sys/socket.h>
#include <sys/types.h>          // 涉及代码：ssize_t, size_t, pid_t, socklen_t
#include <arpa/inet.h>    // 涉及代码：htons(主机转网络序), ntohs, inet_pton(IP转二进制), inet_ntop(二进制转IP), ine
#include <sys/fcntl.h>     // 涉及代码：fcntl, O_NONBLOCK (非阻塞标志)
#include <sys/epoll.h>
#include <netinet/tcp.h>      // TCP_NODELAY需要包含这个头文件。
#include "InetAddress.h"
#include <unistd.h>

int createnonblocking();

class Socket
{
private:
	const int fd_;  //每个socket都有一个fd
	std::string ip_;    //存放ip
	uint16_t port_;
public:
	Socket(int fd);
	~Socket();

	int fd() const; //返回一个fd
	std::string ip() const;
	uint16_t port() const;
	//设置ip和端口
	void set_ip_port(const std::string& ip, uint16_t port);
 
	void setreuseaddr(bool on);
	void setreuseport(bool on);
	void settcpnodelay(bool on);
	void setkeepalive(bool on);

	void bind(const InetAddress& servaddr);
	void listen(int n =128);
	int accept(InetAddress& clientaddr);
};

