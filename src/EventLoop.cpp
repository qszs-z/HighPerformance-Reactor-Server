#include "EventLoop.h"
#include "Epoll.h"   // 必须加！这是为了让cpp文件认识Epoll的具体实现
#include "Channel.h" // 最好也加上，因为你调用了 ch->handleevent
#include <vector>    // 必须加！否则报错 error: ‘vector’ is not a member of ‘std’
#include <sys/syscall.h>
#include <iostream>
#include <unistd.h>

class Epoll;

int createtimefd(int sec = 30) {
	int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);   // 创建timerfd。
	struct itimerspec timeout;                                // 定时时间的数据结构。
	memset(&timeout, 0, sizeof(struct itimerspec));
	timeout.it_value.tv_sec = sec;                             // 定时时间为5秒。
	timeout.it_value.tv_nsec = 0;
	timerfd_settime(tfd, 0, &timeout, 0);                  // 开始计时。alarm(5)
	return tfd;
}

EventLoop::EventLoop(bool mainloop, int timetvl, int timeout)
	:ep_(new Epoll), wakeupfd_(eventfd(0,EFD_NONBLOCK))
	,wakechannel_(new Channel(this,wakeupfd_))
	,timefd_(createtimefd(timetvl_)),timerchannel_(new Channel(this, timefd_))
	,mainloop_ (mainloop),timetvl_(timetvl),timeout_(timeout),stop_(false)
{
	wakechannel_->setreadcallback([this]() {
		handlewakeup();
		});
	//下面是把这个添加到红黑树上
	wakechannel_->enablereading();

	timerchannel_->setreadcallback([this]() {
		handletimer();
		});
	timerchannel_->enablereading();
}

EventLoop::~EventLoop() {
	//delete ep_;
}

void EventLoop::run() {
	thread_ = syscall(SYS_gettid);
	//测试线程的id
	//printf("EventLoop::run() thread is %ld.\n", syscall(SYS_gettid));
	while (stop_ == false) {
		//EventLoop::run()中
		//执行loop  里面有epollwait 然后 触发读事件了 设置revent 之后下面
		//执行ch->handleevent(); 回调读事件
		std::vector<Channel*>channels = ep_->loop(10 * 1000);
		//如果channels为空 表示超时，回调 Tcpserver::epolltimeout();
		if (channels.size() == 0) {
			epolltimeoutcallback_(this);
		}
		else {
			for (auto& ch : channels) {
				ch->handleevent();     //处理返回epoll_wait()返回事件
			}
		}
	}
}
//使用标志位
void EventLoop::stop() {
	stop_ = true;
	//事件循环阻塞在epoll_wait中就算已经是true了也没有反应
	//只有当闹钟响了或者超时也会返回
	wakeup();  //唤醒事件循环
}

void EventLoop::updatechannel(Channel* ch) {
	ep_->updatechannel(ch);
}

void EventLoop::removechannel(Channel* ch)
{
	ep_->removechannel(ch);
}

void EventLoop::setepolltimeoutcallback(std::function<void(EventLoop*)> fn)
{
	epolltimeoutcallback_ = fn;
}
bool EventLoop::isinloopthread() {
	return thread_ == syscall(SYS_gettid);
}

void EventLoop::queueinloop(std::function<void()> fn)
{
	{
		std::lock_guard<std::mutex> gd(mutex_);
		taskqueue_.push(fn);
	}
	wakeup();  //唤醒事件循环

}

void EventLoop::wakeup()
{
	uint64_t val = 1;
	//只要能唤醒就行
	write(wakeupfd_, &val, sizeof(val));
}

void EventLoop::handlewakeup()
{
	//std::cout << "handlewakeup thread is " << syscall(SYS_gettid) << std::endl;
	uint64_t val;
	read(wakeupfd_, &val, sizeof(val)); //从eventfd中读取出数据，如果是水平触发模型会一直触发读事件如果没有读的话。

	std::vector<std::function<void()>> temp_tasks;
	{
		// 2. 临界区：只负责把队列里的东西“偷”出来
		std::lock_guard<std::mutex> gd(mutex_);
		while (!taskqueue_.empty()) {
			temp_tasks.emplace_back(std::move(taskqueue_.front()));
			taskqueue_.pop();
		}
	} // 3. 锁在这里就释放了！

	// 4. 在锁外面执行任务
	// 这样 fn() 就算执行很久，或者再调用 queueinloop，也不会卡死别人
	for (const auto& func : temp_tasks) {
		func();
	}
}
void EventLoop::handletimer()
{
	//在 handletimer 函数的最开头，一定要把 timerfd 的数据读走！ 
	// 否则 epoll 会因为数据没读完，一直死循环触发（Level Trigger），导致 CPU 100%。
	uint64_t val;
	ssize_t n = read(timefd_, &val, sizeof(val));
	if (n != sizeof(val)) { /* 错误处理 */ }
	//重新计时
	struct itimerspec timeout;                                // 定时时间的数据结构。
	memset(&timeout, 0, sizeof(struct itimerspec));
	timeout.it_value.tv_sec = timetvl_;                             // 定时时间为timetvl_秒。
	timeout.it_value.tv_nsec = 0;
	timerfd_settime(timefd_, 0, &timeout, 0);                  // 开始计时。alarm(5)
	if (mainloop_) {
		//std::cout << "main loop alarm start" << std::endl;
	}
	else {
		// 1. 【开头】先打印线程 ID
		//printf("EventLoop::handletimer() thread is %ld fd: ", syscall(SYS_gettid));
		time_t now = time(0);
		std::vector<int> timeout_fds;

		// ==========================================================
		// 【第一步：快速收集】锁住整个 map
		// ==========================================================
		{
			std::lock_guard<std::mutex> gd(mmutex_);
			for (auto& pair : conns_) {
				// 2. 【中间】在这里打印遍历到的每一个 fd
				// pair.first 就是 map 的 key，也就是 fd
				//printf("%d ", pair.first);
				if (pair.second->timeout(now, timeout_)) {
					timeout_fds.push_back(pair.first); // 记录超时的
				}
			}
		} // 解锁
		// 3. 【结尾】循环结束后换行
		//printf("\n");
		// ==========================================================
		// 【第二步：安全处决】(代码不变)
		// ==========================================================
		for (int fd : timeout_fds) {
			timecallback_(fd);
			{
				std::lock_guard<std::mutex> gd(mmutex_);
				conns_.erase(fd);
			}
		}
	}
}
void EventLoop::newconnection(spConnection conn)
{
	{
		std::lock_guard<std::mutex> gd(mmutex_);
		conns_[conn->fd()] = conn;
	}
}
void EventLoop::settimecallback(std::function<void(int)> fn)
{
	timecallback_ = fn;
}
//wakechannel_->enablereading(); 把读事件加到里面红黑树
//设置读事件的回调
//wakechannel_->setreadcallback([this]() {
//handlewakeup();});

//也就是说channel监听到 wakefd 有读事件 write是吗？
// 然后就会触发handlewakeup()，然后执行read取消报警，执行下面的任务队列中的函数
