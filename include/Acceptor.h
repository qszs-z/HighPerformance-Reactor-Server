#pragma  once
#include <memory>
#include <string>
#include <cstdint>
#include <memory>
#include <functional>
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class Socket;
class Channel;

class Acceptor{
public:
	Acceptor(EventLoop* loop, const std::string& ip, const uint16_t port);
	~Acceptor();
	void newconnection(); // servsock 是成员变量不需要了

	void setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn);

private:
	EventLoop* loop_;       //Acceptor 对应的事件循环，在构造函数中传入
	//const std::unique_ptr<EventLoop>& loop_;  //不能使用移动语义
	//loop_在Tcp_server中  从外面传进来  不要在析构函数中释放了
	Socket servsock_;     //在构造函数中创建的 用std::unique_ptr
	//std::unique_ptr<Channel> acceptchannel_;   这两个类不大可以不用智能指针
	Channel  acceptchannel_;       //这里是因为一个网络服务程序只有一个acceptor  所以这里是用的栈
	//而connnection中不是使用的指针
	std::function<void(std::unique_ptr<Socket>)> newconnectioncb_;
};