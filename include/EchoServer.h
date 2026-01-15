#pragma  once
#include "TcpServer.h"
#include "EventLoop.h"
#include "Connection.h"
#include "ThreadPool.h"
class Connection;  // 告诉编译器：Connection 是个类，具体的长相在下面，你先别急。
using spConnection = std::shared_ptr<Connection>; // 现在编译器就不报错了

class EchoServer {
private:
	TcpServer tcpserver_;
	ThreadPool threadpool_;   //工作线程池
public:
	EchoServer(const std::string& ip, const uint16_t port,int subthreadnum = 3,int workthreadnum = 5,uint16_t sep = 1);   //初始化tcpserver时要传入ip和port
	~EchoServer();

	void Start();
	void Stop();

	void HandleNewConnection(spConnection conn);
	void HandleClose(spConnection conn);
	void HandleError(spConnection conn);    //客户端连接发生错误的时候，在Tcpserver类中回调此函数。
	void HandleMessage(spConnection conn, std::string & message); //处理客户端请求报文
	//在Connection类中回调函数
	void HandleSendComplete(spConnection cnn);  //数据发送完成之后，再Tcpserver类中回调函数
	//void HandleTimeOut(EventLoop* loop);  // 现在的tcpserver只有一个事件循环之后有多个
	//谁要回调参数就是谁
	//添加业务处理的函数
	void OnMessage(spConnection conn, std::string& message);
};