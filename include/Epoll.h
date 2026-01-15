#pragma once
#include <sys/socket.h>
#include <sys/types.h>          // 涉及代码：ssize_t, size_t, pid_t, socklen_t
#include <arpa/inet.h>    // 涉及代码：htons(主机转网络序), ntohs, inet_pton(IP转二进制), inet_ntop(二进制转IP), ine
#include <sys/fcntl.h>     // 涉及代码：fcntl, O_NONBLOCK (非阻塞标志)
#include <sys/epoll.h>
#include <netinet/tcp.h>      // TCP_NODELAY需要包含这个头文件。
#include "InetAddress.h"
#include <unistd.h>
#include <vector>
#include <strings.h>
// #include "Channel.h"  <--- 1. 删掉这个！不要在这里包含

class Channel; 
//Epoll 类
class Epoll {
private:
	static const int MaxEvents = 100;
	int epollfd_ = -1;                     //Epoll 句柄
	epoll_event events_[MaxEvents];
public:
	Epoll();              //构造函数中创建epoll_fd
	~Epoll();            //析构函数中关闭epoll_fd

	//void addfd(int fd, uint32_t op);    //把fd和它需要监视的时间添加到红黑树上
	void updatechannel(Channel* ch);   //把channel 添加/更新到红黑树中 channel 中有 fd
	void removechannel(Channel* ch);      //删除
	std::vector<Channel*> loop(int timeout = -1); //用容器返回已发生的事件
	//程序的心跳。对应系统调用 epoll_wait 的超时时间。

};