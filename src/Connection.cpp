#include "Connection.h"
#include "Channel.h"
#include "Socket.h"
#include <sys/syscall.h>
#include "EventLoop.h"
//左值 (外部) -> move -> 右值 (传递) -> 变左值 (形参) -> move -> 右值 (给成员)
//刚才那个悬空的资源，被塞进了 clientsock 这个变量里。变左值 (形参)  然后还得move
Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> clientsock, uint16_t sep)
	:loop_(loop), clientsock_(std::move(clientsock)), clientchannel_(), disconnect_(false)
	,inputbuffer_(sep), outputbuffer_(sep)
{
	clientchannel_= std::make_unique<Channel> (loop_, clientsock_->fd());   
	clientchannel_->setreadcallback(std::bind(&Connection::onmessage, this));
	clientchannel_->setclosecallback(std::bind(&Connection::closecallback, this));
	clientchannel_->seterrorcallback(std::bind(&Connection::errorcallback, this));
	clientchannel_->setwritecallback([this]() {
		this->writecallback();
		});
	clientchannel_->useet();   //边缘触发    写事件是只要不满就一直会触发
	clientchannel_->enablereading(); //enablereading 里有 updatechannel 可以添加事件
}
//spConnection conn = new Connection(loop_, std::move(clientsock)); 这个右值被传递给构造函数，
// 到Connection(EventLoop* loop, std::unique_ptr<Socket> clientsock);这里 clientsock变成左值
// 然后std::move(clientsock)再把它转成右值 来传递unique——ptr

Connection::~Connection()
{
	//delete clientsock_; 智能指针不用关了
	//std::cout << "conn has been released." << std::endl;
}
int Connection::fd() const {

	return clientsock_->fd();
}//返回一个fd
std::string Connection::ip() const {
	return clientsock_->ip();
}
uint16_t Connection::port() const {
	return clientsock_->port();
}

void Connection::closecallback()
{
	disconnect_ = true;
	//printf("client(event_fd=%d) disconnected.\n", fd());
	//close(fd());            // 关闭客户端的fd。
	clientchannel_->remove();
	closecallback_(shared_from_this());
}

void Connection::errorcallback()
{
	disconnect_ = true;
	//printf("client(event_fd=%d) error.\n", fd());
	//close(fd());            // 关闭客户端的fd。
	clientchannel_->remove();
	errorcallback_(shared_from_this());
}


void Connection::setclosecallback(std::function<void(spConnection)> fn) {
	closecallback_ = fn;
}
//void TcpServer::closeconnection(spConnection conn)
// //这个对象被存到了 Connection 的 closecallback_ 变量里。
//conn->setclosecallback(std::bind(&TcpServer::closeconnection, this,std::placeholders::_1));
//	closecallback_(this);     这个相当于调用  TcpServer::closeconnection(this)吗？
void Connection::seterrorcallback(std::function<void(spConnection)> fn) {
	errorcallback_ = fn;
}


void Connection::setonmessagecallback(std::function<void(spConnection, std::string&)> fn)
{
	onmessagecallback_ = fn;
}

void Connection::setsendcompletecallback(std::function<void(spConnection)> fn)
{
	sendcompletecallback_ = fn;
}

bool Connection::timeout(time_t now,int val)
{
	return (now - lastatime_.toint()) > val;
}

void Connection::onmessage()
{
	//不用临时变量
	char buf[1024];
	while (true) {
		memset(buf, 0, sizeof(buf));
		ssize_t n = read(fd(), buf, sizeof(buf));
		if (n > 0) {
			// 把接收到的报文内容原封不动的发回去。
			//printf("recv(event_fd=%d):%s\n", fd(), buf);
			//send(fd(), buf, n, 0);
			inputbuffer_.append(buf,n);  //把读取的追加的发送缓冲区中
			
		}
		else if (n == -1 && errno == EINTR) {// 读取数据的时候被信号中断，继续读取。
			continue;
		}
		else if (n == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) { // 全部的数据已读取完毕。
			//读取完毕之后再发送给对端
			//printf("recv(event_fd=%d):%s\n", fd(), inputbuffer_.data());
			std::string message;
			while (true) {
				//接受数据   从接受缓冲区读取
				///////////////////////////////////把下面封装到buffer中
				if (inputbuffer_.pickmesssage(message) == false) break;
				//int len;
				//memcpy(&len, inputbuffer_.data(), 4);
				//if (inputbuffer_.size() < len + 4) break;
				////5abcde     取4个字节   包头[0x00 0x00 0x00 0x05] (4 bytes)   数据[a] [b] [c] [d] [e] (5 bytes)
				////8abcdefeh  
				////6 abc   小于6 加 4  不完整跳出  留在接受缓冲区中
				//std::string message(inputbuffer_.data() + 4, len); //获取一个报文
				//inputbuffer_.erase(0, len + 4);  //  删除刚刚读取的报文
				////////////////////////////////////////////////////
				//数据计算的工作放到tcpserver中
				//printf("message (event_fd=%d):%s\n", fd(), message.c_str());
				lastatime_ = Timestamp::now();
				//std::cout << "lastatime =" << lastatime_.tostring() << std::endl;
				//message = "reply:" + message;

				//len = message.size();
				//std::string tmpbuf((char *) &len, 4);  //把报文头部填充到会有报文
				//tmpbuf.append(message);                //把报文内容填充到回应报文
				////发送数据
				//send(fd(), tmpbuf.data(), tmpbuf.size(), 0);
				onmessagecallback_(shared_from_this(), message);  //回调TcpServer::onmeaasge();
			}
			break;
		}
		else if (n == 0) {// 客户端连接已断开。
			/*	printf("client(event_fd=%d) disconnected.\n", fd_);
				close(fd_);*/
			//clientchannel_->remove();
			closecallback();  //回调
			//clientchannel_->setclosecallback(std::bind(&Connection::closecallback, this));
			//closecallback_();调用的是 Connection::closecallback
			//
			break;
		}
	}
}

void Connection::send(const char* data, size_t size)
{
	//printf("Connection::send thread is %ld.\n", syscall(SYS_gettid));
	if (disconnect_ == true) { 
		//std::cout << "client is disconnection, send() return\n";
		return;
	}
	if (loop_->isinloopthread()) {    //判断当时是不是在io线程
		//outputbuffer_.appendwithhead(data, size);
		//clientchannel_->enablewriting();   //注册写事件  //边缘触发
		//std::cout << "send()  in io thread" << std::endl;
		sendinloop(data, size);
	}
	else {  //如果不是io线程哈  封装上面的两个代码
		// 调用Event loop：queueinloop 把这个交给事件循环去执行。
		//std::cout << "send() no in io thread" << std::endl;
		//注意：此时没有复制数据内容，只复制了那个“地址条”。
		//[销毁] 局部变量 message (0x100) 被销毁！内存被标记为可用。
		//执行任务] 执行 sendinloop(data)。它拿着地址条 0x100 去找数据。但是 0x100 早就已经在第 7 步被销毁了！
		//loop_->queueinloop([this, data, size]() {
		//	sendinloop(data, size);
		//	});
		loop_->queueinloop([this, message = std::string(data, size)]() {
			sendinloop(message.data(), message.size());
			});
		//conn->send(message.data(), message.size())。
		//OnMessage 函数结束。关键点来了： 局部变量 message 被销毁了！ 它的内存被释放了。
		//Lambda 里存的那个 data 指针，指向了一块非法内存（或者是已经被别的变量占用的内存）。
	}
	
}
void Connection::sendinloop(const char* data, size_t size) {
	outputbuffer_.appendwithsep(data, size);
	clientchannel_->enablewriting();   //注册写事件  //边缘触发
}

void Connection::writecallback()
{
	int writen = ::send(fd(), outputbuffer_.data(), outputbuffer_.size(), 0);
	if (writen > 0) outputbuffer_.erase(0, writen);   //删除已经发送的字符
	//printf("Connection::writecallback thread is %ld.\n", syscall(SYS_gettid));
	//如果发送缓冲区中没有数据了就不再关注写事件   面经里记得一样 边缘触发写完
	//停止写事件
	if (outputbuffer_.size() == 0) {
		clientchannel_->disablewriting();
		sendcompletecallback_(shared_from_this());
	}
}

//这是因为 “epoll 在哪里等，回调就在哪里跑”。
//场景还原：
//刚才工作线程里的 send 做了一件事：clientchannel_->enablewriting()。这相当于告诉内核：“我要发数据，等网卡空闲了叫我”。
//IO 线程 一直在 EventLoop 里转圈，卡在 epoll_wait 那里睡觉。
//一旦网卡准备好了（或者你刚注册了写事件且缓冲区是空的），内核通知 epoll_wait 返回。
//IO 线程 醒来，发现是“写事件”触发了。
//IO 线程 顺藤摸瓜找到 Channel，调用了你注册的 writecallback。
//结论：writecallback 是由 EventLoop 调用的，而 EventLoop 永远属于 IO 线程。