#pragma once
#include <sys/epoll.h>
#include <functional>
#include <memory>
//#include "InetAddress.h"
//#include <iostream>
//#include "EventLoop.h"只声明 class Epoll; class Channel;  可以放到cpp中
class Socket;
class EventLoop;
class InetAddress; // 其实这行都可以不要，因为头文件里甚至没出现这个词

class Channel {                    //channel是acceptor和connection的下层类 
private:
	int fd_ = -1;        //Channel 拥有的fd,Channel 和fd 是一对一的关系
	//这里没有创建实例 只是声明
	//不属于自己、但会使用的资源,采用unique_ptr
	//&或shared_ptr来管理会很麻烦、不易与阅读，还可能会对新手带来一系列问题依旧采用裸指针来管理。
	EventLoop *loop_ =  nullptr;  //Channel 对应的红黑树 Channel与epoll是多对一的关系，一个channel只对应一个EPOLL
	//const std::unique_ptr<EventLoop>& loop_;
	bool inepoll_ = false;  //是否有事件添加到epoll上 
	uint32_t events_ = 0;  //fd_需要监视的事件
	uint32_t revents_ = 0;  //fd_已发生的事件
	//bool islisten_ = false;  //如果是listenfd 取值为true 客户端连接上来的fd取值false   有回调函数了不用了
	std::function<void()> readcallback_;         // fd_读事件的回调函数，如果是acceptchannel，将回调Acceptor::newconnection()，如果是clientchannel，将回调Channel::onmessage()。
	std::function<void()> closecallback_;        // 关闭fd_的回调函数，将回调Connection::closecallback()。
	std::function<void()> errorcallback_;        // fd_发生了错误的回调函数，将回调Connection::errorcallback()。
	std::function<void()> writecallback_;     //回调connect中的
public:
	Channel(EventLoop* loop,int fd);
	~Channel();

	int fd();       //返回fd_成员

	void useet();      //采用边缘触发
	
	void enablereading();  //让epoll_wait()监视fd_的读事件
	void disablereading();  //让epoll_wait()监视fd_的读事件
	void enablewriting();  //让epoll_wait()监视fd_的读事件
	void disablewriting();  //让epoll_wait()监视fd_的读事件
	void disableall();        //取消全部的事件    当tcp断开是
	void remove();           //从事件循环中删除channel。
	void setinepoll();         //把inepoll_成员设置成true
	void setrevents(uint32_t ev);   //设置revents 的成员
	bool inepoll();       //返回 inepoll 是否有事件添加
	uint32_t events();   // 返回 events
	uint32_t revents();   // 返回 revents

	void handleevent();

	//void newconnection(Socket* servsock);
	//void onmessage();

	void setreadcallback(std::function<void()>fn);  //设置fd_读事件的回调函数
	//使用std::bind std::bind 返回的类型： 一个调用时不需要参数的对象（因为它内部已经存好了所需的参数）。
	//std::bind 的核心作用：“提前传参”

	void setclosecallback(std::function<void()>fn);
	void seterrorcallback(std::function<void()>fn);
	void setwritecallback(std::function<void()>fn);
};