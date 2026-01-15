#pragma once
#include <cstdint>
#include <memory>
#include <functional>
#include "Buffer.h"
#include <memory>
#include <atomic>
#include "Timestamp.h"
class EventLoop;
class Socket;
class Channel;
class Connection;

using spConnection = std::shared_ptr<Connection>;  //要在这个之前来声明connection

class Connection:public std::enable_shared_from_this <Connection>{
public:
	Connection(EventLoop* loop, std::unique_ptr<Socket> clientsock, uint16_t sep);
	~Connection();
	int fd() const; //返回一个fd
	std::string ip() const;
	uint16_t port() const;
	
	void send(const char* data, size_t size);
	void sendinloop(const char* data, size_t size);

	void onmessage();              //回调函数
	void closecallback();             //TCP连接关闭       供channel回调
	void errorcallback();             //TCP连接错误        供channel回调
	void writecallback();             //写事件的回调函数  
	void setclosecallback(std::function<void(spConnection)> fn);
	void seterrorcallback(std::function<void(spConnection)> fn);
	void setonmessagecallback(std::function<void(spConnection, std::string&)> fn);
	void setsendcompletecallback(std::function<void(spConnection)> fn);

	bool timeout(time_t now,int val);
private:
	EventLoop* loop_;       //Acceptor 对应的事件循环，在构造函数中传入
	//const std::unique_ptr<EventLoop>& loop_;
	//loop_在Tcp_server中  从外面传进来  不要在析构函数中释放了
	//Socket * clientsock_;  //这个是传过来的  clientsock 和 connection 声明周期一样。
	std::unique_ptr<Socket> clientsock_;  //拿到了所有权 move 这个从外面传进来的  但是在这个类中析构
	std::unique_ptr<Channel> clientchannel_;  //  在构造函数中建立

	std::function<void(spConnection)> closecallback_;
	std::function<void(spConnection)> errorcallback_;
	//std::function<void(spConnection)> writecallback_;
	std::function<void(spConnection,std::string&)> onmessagecallback_;
	std::function<void(spConnection)> sendcompletecallback_;
	Buffer inputbuffer_;
	Buffer outputbuffer_;

	std::atomic_bool disconnect_;   //客户端连接是否断开
	//在io线程中会改变这个的值，在工作线程中判断

	Timestamp lastatime_;   //创建connection 对象时时间戳时当时的时间，每收到一个报文更新时间

};