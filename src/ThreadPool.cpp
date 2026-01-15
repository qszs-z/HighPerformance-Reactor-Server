#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadnum, const std::string& threadtype) 
	:stop_(false),threadtype_(threadtype)
{
	for (size_t i = 0; i < threadnum; i++) {
		threads_.emplace_back([this] {
			//显示线程id
			printf("create %s thread(%ld).\n", threadtype_.c_str(), syscall(SYS_gettid));

			while (stop_ == false) {
				std::function<void()>task;
				{//锁作用域开始
					std::unique_lock<std::mutex> lock(this->mutex_);
					//std::lock_guard 没有 lock() 和 unlock() 接口！它做不到“先解锁再加锁”这种动作。
					//只有 std::unique_lock 才有这个能力配合 wait 函数进行这种“反复横跳”的操作。
					//等待生成者的条件变量
					this->condition_.wait(lock, [this] {
						return ((this->stop_ == true) || (this->taskqueue_.empty() == false));
						});
					//第二个参数是条件，如果为true则往下执行。
					if ((this->stop_ == true) || (this->taskqueue_.empty() == true)) return;
					//出队一个任务   使用移动语义避免拷贝。
					task = std::move(this->taskqueue_.front());
					//出队之后删除已出队的
					this->taskqueue_.pop();
				}
				//printf("%s(%ld) execute task.\n", threadtype_.c_str(), syscall(SYS_gettid));
				task();  //执行任务。
			}
			});
	}
}
//进大括号 {：unique_lock 创建，自动上锁。
//出大括号 }：unique_lock 销毁（调用析构函数），自动解锁。
//尽量缩短持有锁的时间，只在“取任务”的那一瞬间加锁，
//千万不要在“干活”的时候也拿着锁
void ThreadPool::addtask(std::function<void()> task)
{
	{
		std::lock_guard<std::mutex> lock(mutex_);  //上锁
		taskqueue_.push(task);
	}   //解锁
	condition_.notify_one();   //唤醒一个线程，队列里是空的线程会等待被唤醒。
}


size_t ThreadPool::size() {
	return threads_.size();
}

void ThreadPool::stop() {
	if (stop_) return;
	stop_ = true;
	condition_.notify_all();   //唤醒全部 执行下面的代码
	//队列是空就 返回了
	for (std::thread& th : threads_) {
		th.join();   //等待全部线程退出
	}
}

//只要 stop_ 是 false，这个 while 就永远不会结束，子线程也就永远不会退出。
ThreadPool::~ThreadPool()
{
	//调用上面的停止函数
	stop();
}

//void show(int no, const std::string& name) {
//	printf("小哥哥们好，我是第%d号超级女生%s。\n", no, name.c_str());
//}
//void test()
//{
//	printf("我有一只小小鸟。\n");
//}
//
//int main() {
//
//	ThreadPool threadpool(3);
//	std::string name = "西施";
//	threadpool.addtask([name] {
//		show(8, name);
//		});
//	sleep(1);
//	threadpool.addtask([] {
//		test();
//		});
//	sleep(1);
//	threadpool.addtask([] {
//		printf("我是一只傻傻鸟。\n");
//		});
//	sleep(1);
//	return 0;
//}
