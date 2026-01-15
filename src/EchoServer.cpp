#include "EchoServer.h"


EchoServer::EchoServer(const std::string& ip, const uint16_t port, int subthreadnum,int workthreadnum, uint16_t sep)
				:tcpserver_(ip, port, subthreadnum,sep) ,threadpool_(workthreadnum,"WORKS")
{
	//业务关心是吗就指定对应的回调函数。
	tcpserver_.setnewconnetioncb(
		[this](spConnection conn) {
			this->HandleNewConnection(conn);
		});
	tcpserver_.setcloseconnectioncb(
		[this](spConnection conn) {
			this->HandleClose(conn);
		}
	);
	tcpserver_.seterrorconnectioncb(
		[this](spConnection conn) {
			this->HandleError(conn);
		}
	);
	tcpserver_.setonmessagecb(
		[this](spConnection conn, std::string& message) {
			this->HandleMessage(conn, message);
		}
	);
	tcpserver_.setsendcompletecb(
		[this](spConnection conn) {
			this->HandleSendComplete(conn);
		}
	);
	//tcpserver_.settimeoutcb(
	//	[this](EventLoop* loop) {
	//		this->HandleTimeOut(loop);
	//	}
	//);
}

EchoServer::~EchoServer()
{

}

void EchoServer::Start() {
	tcpserver_.start();
}
void EchoServer::Stop()   //停止服务
{
	//停止工作线程
	threadpool_.stop();
	std::cout << "work thread stop..." << std::endl;
	//停止io线程
	tcpserver_.stop();   //停止主从事件循环和io线程
	//std::cout << "IO thread stop..." << std::endl;
}
void EchoServer::HandleNewConnection(spConnection conn) {
	std::cout << Timestamp::now().tostring().c_str()<< "new connection(fd=" << conn->fd()
		<< ",ip=" << conn->ip()   // 这里的 ip() 返回 std::string 也没问题！
		<< ",port=" << conn->port()
		<< ") ok." << std::endl;
	//printf("EchoServer::HandleNewConnection thread is %ld.\n", syscall(SYS_gettid));
	//std::cout << "New Connection Come in" << std::endl;
}
void EchoServer::HandleClose(spConnection conn) {
	//std::cout << "EchoServer conn closed" << std::endl;
	std::cout << Timestamp::now().tostring().c_str() << "connection closed(fd=" << conn->fd()
		<< ",ip=" << conn->ip()   // 这里的 ip() 返回 std::string 也没问题！
		<< ",port=" << conn->port()
		<< ") ok." << std::endl;
}
void EchoServer::HandleError(spConnection conn)    //客户端连接发生错误的时候，在Tcpserver类中回调此函数。
{
	std::cout << "EchoServer conn error" << std::endl;
}
void EchoServer::HandleMessage(spConnection conn, std::string & message) //处理客户端请求报文
{
	//printf("EchoServer::HandleMessage thread is %ld.\n", syscall(SYS_gettid));
	
	//封装到buffer类中
	//int len = message.size();
	//std::string tmpbuf((char*)&len, 4);  //把报文头部填充到会有报文
	//tmpbuf.append(message);                //把报文内容填充到回应报文
	//message = "reply:" + message;     //回显业务
	//发送数据
	//conn->send(message.data(), message.size());  这个函数搬到了下面的函数里了。
	if (threadpool_.size() == 0) {  //工作线程时0
		//工作线程时0,在io线程中计算
		OnMessage(conn, message);
	}
	else {//把业务添加到工作线程池的任务队列中
		//threadpool_.addtask 需要的是一个 “无参函数” ( void() )。  不能写参数[this,conn,message](spConnection conn, std::string& message)
		//使用lambda表达式，编译器把std::string &它变成了 const std::string message
		//mutable 取消了const属性
		threadpool_.addtask([this, conn, message]() mutable {
			this->OnMessage(conn, message);
			});
	}
}
void EchoServer::OnMessage(spConnection conn, std::string& message) {
	//printf("%s message (event_fd=%d):%s\n", Timestamp::now().tostring().c_str(), conn->fd(), message.c_str());
	message = "reply:" + message;     //回显业务
	//sleep(2);
	//std::cout << "After the processing is completed, the conn object used here has been released.\n";
	conn->send(message.data(), message.size());
}
//在Connection类中回调函数
void EchoServer::HandleSendComplete(spConnection cnn)//数据发送完成之后，再Connection类中回调函数
{
	//std::cout << "Message send complete." << std::endl;
}
//void EchoServer::HandleTimeOut(EventLoop* loop)  // 现在的tcpserver只有一个事件循环之后有多个
//{
//	std::cout << "EchoServer timeout." << std::endl;
//}