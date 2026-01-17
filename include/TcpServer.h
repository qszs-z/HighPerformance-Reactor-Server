#pragma once
#include "EventLoop.h"
//#include <unistd.h>  //Linux 独有的 不进行 read/write/close 操作
#include <memory>
#include <string>
#include <cstdint>
#include <iostream>
#include<map>
#include<vector>
#include "ThreadPool.h"
#include "Acceptor.h"
#include <mutex>
class Acceptor;
class Socket;
class Connection;  // 告诉编译器：Connection 是个类，具体的长相在下面，你先别急。

using spConnection = std::shared_ptr<Connection>; // 现在编译器就不报错了

class TcpServer {
private: 
	//主事件循环
	//EventLoop *mainloop_;   //一个tcpserver可以有多个事件循环，现在是单线程，暂时只有一个事件循环。
	std::unique_ptr<EventLoop>mainloop_;
	//std::vector<EventLoop*>subloops_;   //存放从事件循环的容器
	std::vector<std::unique_ptr<EventLoop>>subloops_; 
	
	int threadnum_;//线程池的大小，从事件循环的个数，一个线程运行一个事件循环
	//ThreadPool* threadpool_;                  //线程池 可以不用指针
	ThreadPool threadpool_;
	           
	std::mutex mmutex_;             //保护conns_互斥
	//Acceptor* acceptor_; //一个Tcpserve只有一个Acceotor 就像 一个服务端只有一个listenfd一样。
	//说是这里不用指针  一个Tcpserve只有一个Acceotor
	//std::unique_ptr<Acceptor> acceptor_;   //Acceptor 类应该属于 TcpServer  当
	Acceptor  acceptor_;
	std::map<int, spConnection> conns_;
	std::function<void(spConnection)> newconnectioncb_;    //回调Echo::server的回调函数。
	std::function<void(spConnection)> closeconnectioncb_;
	std::function<void(spConnection)> errorconnectioncb_;
	std::function<void(spConnection conn, std::string& message)> onmessagecb_;
	std::function<void(spConnection)> sendcompletecb_;
	std::function<void(EventLoop* loop)> timeoutcb_;

	std::function<void(int)> removeconnetioncb_;    //BankServer::HandleRemove

	uint16_t sep_;   //报文格式    // 这里需要存一下，因为 Connection 是后来才创建的
public:
	TcpServer(const std::string& ip, const uint16_t port,int threadnum = 3, uint16_t sep = 1);
	~TcpServer();
	void start();   //运行事件循环s
	void stop();   //停止io线程

	void newconnection(std::unique_ptr<Socket> clientsock);
	void closeconnection(spConnection conn);
	void errorconnection(spConnection conn);    //客户端连接发生错误的时候，在Connection类中回调此函数。
	void onmessage(spConnection conn, std::string& message); //处理客户端请求报文
	//在Connection类中回调函数
	void sendcomplete(spConnection conn);  //数据发送完成之后，再Connection类中回调函数
	void epolltimeout(EventLoop* loop);  // 现在的tcpserver只有一个事件循环之后有多个
	//谁要回调参数就是谁


	void setnewconnetioncb(std::function<void(spConnection)>fn);
	void setcloseconnectioncb(std::function<void(spConnection)>fn);
	void seterrorconnectioncb(std::function<void(spConnection)>fn);
	void setonmessagecb(std::function<void(spConnection conn, std::string &message)>);
	void setsendcompletecb(std::function<void(spConnection)>fn);
	void settimeoutcb(std::function<void(EventLoop* loop)>fn);

	void removeconn(int fd);
	void setremoveconnetioncb(std::function<void(int)>fn);
};

//🟢 第一步：打包（Runtime - 绑定时）
//代码：bind(&func, this, _1)
//发生：程序制作了一个“函数包”。
//包里存好了：TcpServer 的地址（死值）。
//包里留了个洞：_1（空位）。
//🔴 第二步：存储
//这个“函数包”被交给了 Connection 对象保管，存在 closecallback_ 变量里。
//🟡 第三步：使用（Runtime - 调用时）
//场景：连接断开了。
//代码：在 Connection 里执行 closecallback_(this)。
//注意：这里的 this 是 Connection 的指针。
//发生：
//程序拿出那个“函数包”。
//程序看到有一个空位 _1。
//程序把你刚刚传进来的 Connection 指针，塞进了那个空位里。
//拼图完成：TcpServer对象 + Connection参数。
//执行：TcpServer::closeconnection(conn) 成功运行！