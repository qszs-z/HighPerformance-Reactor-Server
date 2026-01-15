#pragma  once
#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <map>
#include "Connection.h"
#include <atomic>
class Epoll;
class Channel;
class Connection;
using spConnection = std::shared_ptr<Connection>;

class EventLoop {
private:
	int timetvl_;           //时间间隔
	int timeout_;           //超时事件
	//声明指针    还没有创建   Epoll ep_;这个是创建
	std::unique_ptr<Epoll> ep_;           //每个事件循环只有一个epoll
	std::function<void(EventLoop*)> epolltimeoutcallback_;   //epolltimeout的回调函数
	pid_t thread_;             //存放事件循环的id
	std::queue<std::function<void()>> taskqueue_;
	std::mutex mutex_;
	int wakeupfd_;
	std::unique_ptr < Channel> wakechannel_;  //将fd纳入epoll管理中
	int timefd_;
	std::unique_ptr<Channel> timerchannel_; 

	bool mainloop_;
	std::mutex mmutex_;             //保护conns_互斥

	std::map<int, spConnection> conns_;

	std::function<void(int)> timecallback_;              //设置成tcpserver::removeconn

	std::atomic_bool stop_;
public:
	EventLoop(bool mainloop, int timetvl= 30, int timeout = 80);                  //在构造函数中创建Epool
	~EventLoop();				 //析构函数中销毁ep_

	void run();              //运行事件循环
	void stop();
	//Epoll* ep();                  //返回epoll类  Channel 的构造函数现在改成了要 EventLoop*。
	void updatechannel(Channel* ch);                        // 把channel添加/更新到红黑树上，channel中有fd，也有需要监视的事件。
	void removechannel(Channel* ch);                        // 把channel删除红黑树上，channel中有fd，也有需要监视的事件。
	void setepolltimeoutcallback(std::function<void(EventLoop*)> fn);
	bool isinloopthread(); //判断当前线程是否为事件循环线程 io线程
	void queueinloop(std::function<void()> fn);

	//用eventfd唤醒事件循环 ，唤醒后执行任务的函数。
	void wakeup();

	void handlewakeup();

	void handletimer();

	void newconnection(spConnection conn);

	void settimecallback(std::function<void(int)> fn);
};