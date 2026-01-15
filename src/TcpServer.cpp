#include "TcpServer.h"
#include "Socket.h"
#include "Channel.h"
#include "Acceptor.h"
#include "Connection.h"
TcpServer::TcpServer(const std::string& ip, const uint16_t port, int threadnum, uint16_t sep)
	:threadnum_(threadnum), mainloop_(new EventLoop(true)), acceptor_(mainloop_.get(), ip, port)
	, threadpool_(threadnum_, "IO"),sep_(sep)
	//threadpool_(threadnum_, "IO")  这里不能用成员变量 得用参数 原因是这个变量的声明子啊线程池的后面
//改了顺序就好了啊
{
	/*
	Socket *servsock = new Socket(createnonblocking());  //创建socket    没释放
	InetAddress servaddr(ip, port);
	servsock->setreuseaddr(true);
	servsock->setkeepalive(true);
	servsock->setreuseport(true);
	servsock->settcpnodelay(true);
	servsock->bind(servaddr);
	servsock->listen();
	Channel* servchannel = new Channel(&loop_, servsock->fd());  // 没释放
	//这里设置了回调函数  使用readcallback是会调用 Channel::newconnection,
	servchannel->setreadcallback(std::bind(&Channel::newconnection, servchannel, servsock));
	servchannel->enablereading();
	*/
	mainloop_->setepolltimeoutcallback(
		[this](EventLoop* loop) {
			this->epolltimeout(loop);
		}
	);
	//acceptor_这里是主事件循环    设置setnewconnectioncb
	//TcpServer::newconnection执行 然后还会执行一个回调
	//运行EchoServer::HandleNewConnection 输出主线程id
	//acceptor_ = std::make_unique<Acceptor>(mainloop_, ip, port);
	acceptor_.setnewconnectioncb(std::bind(&TcpServer::newconnection, this, std::placeholders::_1));
	
	//threadpool_ = new ThreadPool(threadnum_,"IO");  //创建线程池
	//创建从事件循环
	for (int i = 0; i < threadnum_; i++) {
		//subloops_.push_back(new EventLoop);    //创建从事件循环。//隐式转换被拒构造函数是 explicit 的，编译器被禁止做这种
		subloops_.emplace_back(new EventLoop(false,5,10));
		subloops_[i]->setepolltimeoutcallback(
			[this](EventLoop* loop) {
				this->epolltimeout(loop);
			}
		);
		subloops_[i]->settimecallback(
			[this](int fd) {
				this->removeconn(fd);
			}
		);
		//怎么在线程池中运行事件循环呢？  在线程池中添加事件循环
		threadpool_.addtask([loop = subloops_[i].get()]() {  //传入对象的地址要普通指针
			loop->run();
			});
	}
	
	// 
	// std::placeholders::_1指的是现在没有clientsock这个变量 不能传  
	//因为 TcpServer::newconnection 需要一个参数，而你现在给不了，
	// 所以告诉编译器：“这个参数以后由 Acceptor 来填！”
}  //这里的 Socket 要在堆上建立，如果构造函数结束  将执行Socket的析构函数  析构函数里面会析构掉服务端的fd。
TcpServer::~TcpServer() {
	////delete acceptor_; 使用智能指针不用删除
	//delete mainloop_;
	//for (auto& c : conns_) {
	//	delete c.second;
	//}
	/*for (auto& loop : subloops_) {
		delete loop;
	}*/
	//delete threadpool_;
	//析构函数”是服务器倒闭（关机）时才执行的。但你的服务器在营业（运行）期间，会有成千上万的客人（连接）来了又走，走了你就得立刻打扫房间（释放内存），不能等几百年后倒闭了再打扫。
	//如果一个客户端断开了应该立即释放
	//在channel这个类中可以直到 客户端断开  但是connection对象不属于channel
	//所有不能在channel中释放 , 要使用回调函数   释放的代码只能写在这里
	//channel  是connection的底层类  connection 是 tcpserver 的底层类
	//channel 可以回调connection的回调函数,
	//connection可以回调 tcpserver 的回调函数
	//在建立中 channel类回调 accrptor的回调函数
	//acceptor回调 tcpserver的回调函数
}
void TcpServer::start()
{
	mainloop_->run();
}
//停止主从事循环和io线程
void TcpServer::stop() {
	//停止主事件循环
	mainloop_->stop();
	std::cout << "mainLoop stop" << std::endl;
	for (int i = 0; i < threadnum_; i++) {
		subloops_[i]->stop();
	}
	std::cout << "subLoop stop" << std::endl;

	//停止io线程
	threadpool_.stop();
	std::cout << "IO thread  stop" << std::endl;
}
//多个线程在处理一个对象时要加锁
//创建connection对象是在主线程，而释放connection对象是在从事件循环
void TcpServer::newconnection(std::unique_ptr<Socket> clientsock) {
	// 1. 【核心】开启极速模式，拒绝 40ms 延迟
	clientsock->settcpnodelay(true);
	// 2. 【推荐】开启 TCP 心跳，客户端死机了能自动断开
	clientsock->setkeepalive(true);
	int fd = clientsock->fd();
	//Connection* conn = new Connection(mainloop_, std::move(clientsock));  //这里也没有释放以后解决
	spConnection conn(new Connection(subloops_[fd % threadnum_].get(), std::move(clientsock), sep_));
	//既使用一个对象，又 move 这个对象。必须拆分！先把该拿的数据拿出来！
	//Connection* conn = new Connection(subloops_[clientsock->fd() % threadnum_], std::move(clientsock));
	//，当你把多个参数写在括号里时，编译器有权决定先做哪个！ 它不一定是从左到右的
	//动作 B 比较快，先做 B ： 你手里的 clientsock 变成了空气（空指针 nullptr）。
	conn->setclosecallback(std::bind(&TcpServer::closeconnection, this, std::placeholders::_1));
	conn->seterrorcallback(std::bind(&TcpServer::errorconnection, this, std::placeholders::_1));
	conn->setonmessagecallback(
		[this](spConnection conn, std::string message) {
			this->onmessage(conn, message);
		}
	);
	conn->setsendcompletecallback(
		[this](spConnection conn) {
			this->sendcomplete(conn);
		}
	);

	{
		std::lock_guard<std::mutex> gd(mmutex_);
		conns_[conn->fd()] = conn;     //把conn存放到tcpserver的map中
	}
	//敌人 A（主线程）：就是现在这里，新连接来了，主线程正在往里 “插” 数据。
	//敌人 B（子线程）：其他的 SubLoop 线程可能正好有一个连接
	// 断开了（超时或关闭），它们正在通过回调函数调用 TcpServer::closeconnection，试图从这里 “删” 数据。
	subloops_[conn->fd() % threadnum_]->newconnection(conn);               //还需要存放的eventloop的map中
	if(newconnectioncb_)newconnectioncb_(conn);  //这个代码得放在下面   等conn创建
	//左边（主线程）：TcpServer 里的 newconnection 直接调用了 subloop->conns_[fd] = conn。
	//这是主线程在写子线程的 map。
    //右边（子线程）：EventLoop 里的 handletimer 正在遍历 conns_。
    //这是子线程在读 / 写自己的 map。如果不加锁： 
	// 一边在遍历，一边在插入，Boom！ 就是你刚才遇到的 Segmentation Fault。
}
//使用回调函数的目的是在TCpserve中创建connection
//一个tcpserver有多个connection
void TcpServer::closeconnection(spConnection conn) {

	if(closeconnectioncb_)closeconnectioncb_(conn);   //这个代码不能放在下面  delete

	
	//close(conn->fd());            // 关闭客户端的fd。
	{
		std::lock_guard<std::mutex> gd(mmutex_);
		conns_.erase(conn->fd());
	}
	//delete conn;
	//当deleteconn是 调用 
	//Connection::~Connection()
	//
	
}
void TcpServer::errorconnection(spConnection conn)    //客户端连接发生错误的时候，在Connection类中回调此函数。
{

	if(errorconnectioncb_)errorconnectioncb_(conn);
	printf("client(event_fd=%d) error.\n", conn->fd());
	//close(conn->fd());            // 关闭客户端的fd。
	{
		std::lock_guard<std::mutex> gd(mmutex_);
		conns_.erase(conn->fd());
	}
	//delete conn;
}

void TcpServer::onmessage(spConnection conn, std::string & message)
{
	//int len;
	//message = "reply:" + message;     //回显业务
	//len = message.size();
	//std::string tmpbuf((char*)&len, 4);  //把报文头部填充到会有报文
	//tmpbuf.append(message);                //把报文内容填充到回应报文
	//
	////发送数据
	// conn->send(tmpbuf.data(), tmpbuf.size());
	////send(conn->fd(), tmpbuf.data(), tmpbuf.size(), 0);
	////send(fd, ...) 的本质确实是把数据从用户态内存
	//// 你的 tmpbuf）拷贝到内核态的 TCP 发送缓冲区（Kernel Send Buffer）。
	////send 只拷贝 拷贝到多少就通知多少
	////假设数据缓冲区你只能拷贝10kb了满了
	////你得把数据先存到 outputBuffer中懂嘛
	if(onmessagecb_)onmessagecb_(conn, message);
}
void TcpServer::sendcomplete(spConnection conn)
{
	//std::cout << "send complete.\n" << std::endl;
	if(sendcompletecb_)sendcompletecb_(conn);
}
void TcpServer::epolltimeout(EventLoop* loop)
{
	//std::cout << "epoll_wait timeout.\n" << std::endl;
	if (timeoutcb_) {
		timeoutcb_(loop);
	}
}
void TcpServer::setnewconnetioncb(std::function<void(spConnection)> fn)
{
	newconnectioncb_ = fn;
}
void TcpServer::setcloseconnectioncb(std::function<void(spConnection)> fn)
{
	closeconnectioncb_ = fn;
}
void TcpServer::seterrorconnectioncb(std::function<void(spConnection)> fn)
{
	errorconnectioncb_ = fn;
}
void TcpServer::setonmessagecb(std::function<void(spConnection conn, std::string & message)> fn)
{
	onmessagecb_ = fn;
}
void TcpServer::setsendcompletecb(std::function<void(spConnection)> fn)
{
	sendcompletecb_ = fn;
}
void TcpServer::settimeoutcb(std::function<void(EventLoop* loop)>fn)
{
	timeoutcb_ = fn;
}
void TcpServer::removeconn(int fd)
{
	{
		std::lock_guard<std::mutex> gd(mmutex_);
		conns_.erase(fd);//从map中删除conn.
	}
	
}
//既然系统都有缓冲区了，为什么我们还要自己在 TcpConnection 里搞个 InputBuffer 和 OutputBuffer？
// 这不是脱裤子放屁——多此一举吗？
//当然不是！这是因为内核的缓冲区不管是“收”还是“发”，都极其“任性”！