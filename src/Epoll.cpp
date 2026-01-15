#include "Epoll.h"
#include "Channel.h" 

Epoll::Epoll()
{
	if ((epollfd_ = epoll_create(1)) == -1) {
		printf("epoll_cerate() faild(%d).\n", errno);
		exit(-1);
	}
}
Epoll::~Epoll()
{
	close(epollfd_);
}
/*
void Epoll::addfd(int fd, uint32_t op)
{
	epoll_event ev;
	ev.data.fd = fd;
	ev.events = op;

	if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
		printf("epoll_ctl() faild(%d).\n", errno);
		exit(-1);
	}
}
*/

void Epoll::updatechannel(Channel* ch)
{
	epoll_event ev;
	ev.data.ptr = ch;
	ev.events = ch->events();
	if (ch->inepoll()) {  //如果channel已经在树上就更新
		if ((epoll_ctl(epollfd_, EPOLL_CTL_MOD, ch->fd(), &ev))== -1) {
			printf("epoll_ctl() faild(%d).\n", errno);
			exit(-1);
		}
	}
	else {   //不在树上就添加
		if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, ch->fd(), &ev) == -1) {
			printf("epoll_ctl() faild(%d).\n", errno);
			exit(-1);
		}
		ch->setinepoll();   //把channel 的inepoll_成员设置成true。
	}
}
void Epoll::removechannel(Channel* ch) {
	if (ch->inepoll()) {  //如果channel已经在树上就更新
		//printf("removechannel\n");
		if ((epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch->fd(), 0)) == -1) {
			printf("epoll_ctl() faild(%d).\n", errno);
			exit(-1);
		}
	}
}
/*
std::vector<epoll_event> Epoll::loop(int timeout)
{
	std::vector<epoll_event> evs;  
	memset(events_, 0,sizeof(events_));
	int infds = epoll_wait(epollfd_, events_, MaxEvents, timeout);

	if (infds < 0) {
		perror("epoll wait() failed"); exit(-1);
	}
	if (infds == 0) {
		printf("epoll_wait() timeout.\n"); return evs;  //返回空的
	}
	for (int i = 0; i < infds; i++) {
		evs.push_back(events_[i]);
	}
	return evs;
}
*/

std::vector<Channel*> Epoll::loop(int timeout) {
	std::vector<Channel*>channels;
	memset(events_, 0, sizeof(events_));
	int infds = epoll_wait(epollfd_, events_, MaxEvents, timeout);

	if (infds < 0) {
		perror("epoll wait() failed"); exit(-1);
	}
	if (infds == 0) {
		//printf("epoll_wait() timeout.\n"); 
		return channels;  //返回空的
	}
	for (int i = 0; i < infds; i++) {
		//evs.push_back(events_[i]);
		Channel* ch = (Channel *)events_[i].data.ptr;
		ch->setrevents(events_[i].events);
		channels.push_back(ch);
	}
	return channels;
}