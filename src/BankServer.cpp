#include "BankServer.h"

BankServer::BankServer(const std::string& ip, const uint16_t port, int subthreadnum,int workthreadnum, uint16_t sep)
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
	tcpserver_.setremoveconnetioncb(
		[this](int fd) {
			this->HandleRemove(fd);
		}
	);
}

BankServer::~BankServer()
{

}

void BankServer::Start() {
	tcpserver_.start();
}
void BankServer::Stop()   //停止服务
{
	//停止工作线程
	threadpool_.stop();
	std::cout << "work thread stop..." << std::endl;
	//停止io线程
	tcpserver_.stop();   //停止主从事件循环和io线程
	//std::cout << "IO thread stop..." << std::endl;
}
void BankServer::HandleNewConnection(spConnection conn) {
	//std::cout << Timestamp::now().tostring().c_str()<< "new connection(fd=" << conn->fd()
	//	<< ",ip=" << conn->ip()   // 这里的 ip() 返回 std::string 也没问题！
	//	<< ",port=" << conn->port()
	//	<< ") ok." << std::endl;
	//printf("BankServer::HandleNewConnection thread is %ld.\n", syscall(SYS_gettid));
	//std::cout << "New Connection Come in" << std::endl;
	// 新客户端连上来时，把用户连接的信息保存到状态机中。
	spUserInfo userinfo(new UserInfo(conn->fd(), conn->ip()));
	{
		std::lock_guard<std::mutex> gd(mutex_);
		usermap_[conn->fd()] = userinfo;                   // 把用户添加到状态机中。
	}
	printf("%s 新建连接(ip=%s).\n", Timestamp::now().tostring().c_str(), conn->ip().c_str());
}
void BankServer::HandleClose(spConnection conn) {
	//std::cout << "BankServer conn closed" << std::endl;
	//std::cout << Timestamp::now().tostring().c_str() << "connection closed(fd=" << conn->fd()
	//	<< ",ip=" << conn->ip()   // 这里的 ip() 返回 std::string 也没问题！
	//	<< ",port=" << conn->port()
	//	<< ") ok." << std::endl;
	 // 关闭客户端连接的时候，从状态机中删除客户端连接的信息。
	printf("%s 连接已断开(ip=%s).\n", Timestamp::now().tostring().c_str(), conn->ip().c_str());
	{
		std::lock_guard<std::mutex> gd(mutex_);
		usermap_.erase(conn->fd());                        // 从状态机中删除用户信息。
	}
}
void BankServer::HandleError(spConnection conn)    //客户端连接发生错误的时候，在Tcpserver类中回调此函数。
{
	//std::cout << "BankServer conn error" << std::endl;
	HandleClose(conn);
}
void BankServer::HandleMessage(spConnection conn, std::string & message) //处理客户端请求报文
{
	//printf("BankServer::HandleMessage thread is %ld.\n", syscall(SYS_gettid));
	
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
bool getxmlbuffer(const std::string& xmlbuffer, const std::string& fieldname, std::string& value, const int ilen = 0)
{
	std::string start = "<" + fieldname + ">";            // 数据项开始的标签。
	std::string end = "</" + fieldname + ">";            // 数据项结束的标签。

	int startp = xmlbuffer.find(start);                     // 在xml中查找数据项开始的标签的位置。
	if (startp == std::string::npos) return false;

	int endp = xmlbuffer.find(end);                       // 在xml中查找数据项结束的标签的位置。
	if (endp == std::string::npos) return false;

	// 从xml中截取数据项的内容。
	int itmplen = endp - startp - start.length();
	if ((ilen > 0) && (ilen < itmplen)) itmplen = ilen;
	value = xmlbuffer.substr(startp + start.length(), itmplen);

	return true;
}
void BankServer::OnMessage(spConnection conn, std::string& message) {
	//printf("%s message (event_fd=%d):%s\n", Timestamp::now().tostring().c_str(), conn->fd(), message.c_str());
	//message = "reply:" + message;     //回显业务
	//sleep(2);
	//std::cout << "After the processing is completed, the conn object used here has been released.\n";
	spUserInfo userinfo = usermap_[conn->fd()];    // 从状态机中获取客户端的信息。
	// 解析客户端的请求报文，处理各种业务。
	// <bizcode>00101</bizcode><username>wucz</username><password>123465</password>
	std::string bizcode;                 // 业务代码。
	std::string replaymessage;      // 回应报文。
	getxmlbuffer(message, "bizcode", bizcode);       // 从请求报文中解析出业务代码。
	if (bizcode == "00101")     // 登录业务。
	{
		std::string username, password;
		getxmlbuffer(message, "username", username);    // 解析用户名。
		getxmlbuffer(message, "password", password);     // 解析密码。
		if ((username == "zqz") && (password == "123456"))         // 假设从数据库或Redis中查询用户名和密码。
		{
			// 用户名和密码正确。
			replaymessage = "<bizcode>00102</bizcode><retcode>0</retcode><message>ok</message>";
			userinfo->setLogin(true);               // 设置用户的登录状态为true。
		}
		else
		{
			// 用户名和密码不正确。
			replaymessage = "<bizcode>00102</bizcode><retcode>-1</retcode><message>用户名或密码不正确。</message>";
		}
	}
	else if (bizcode == "00201")   // 查询余额业务。
	{
		if (userinfo->Login() == true)
		{
			// 把用户的余额从数据库或Redis中查询出来。
			replaymessage = "<bizcode>00202</bizcode><retcode>0</retcode><message>5088.80</message>";
		}
		else
		{
			replaymessage = "<bizcode>00202</bizcode><retcode>-1</retcode><message>用户未登录。</message>";
		}
	}
	else if (bizcode == "00901")   // 注销业务。
	{
		if (userinfo->Login() == true)
		{
			replaymessage = "<bizcode>00902</bizcode><retcode>0</retcode><message>ok</message>";
			userinfo->setLogin(false);               // 设置用户的登录状态为false。
		}
		else
		{
			replaymessage = "<bizcode>00902</bizcode><retcode>-1</retcode><message>用户未登录。</message>";
		}
	}
	else if (bizcode == "00001")   // 心跳。
	{
		if (userinfo->Login() == true)
		{
			replaymessage = "<bizcode>00002</bizcode><retcode>0</retcode><message>ok</message>";
		}
		else
		{
			replaymessage = "<bizcode>00002</bizcode><retcode>-1</retcode><message>用户未登录。</message>";
		}
	}

	conn->send(replaymessage.data(), replaymessage.size());
}
//在Connection类中回调函数
void BankServer::HandleSendComplete(spConnection cnn)//数据发送完成之后，再Connection类中回调函数
{
	//std::cout << "Message send complete." << std::endl;
}
//void BankServer::HandleTimeOut(EventLoop* loop)  // 现在的tcpserver只有一个事件循环之后有多个
//{
//	std::cout << "BankServer timeout." << std::endl;
//}
void BankServer::HandleRemove(int fd)                       // 客户端的连接超时，在TcpServer类中回调此函数。
{
	printf("fd(%d) 已超时。\n", fd);

	std::lock_guard<std::mutex> gd(mutex_);
	usermap_.erase(fd);                                      // 从状态机中删除用户信息。
}