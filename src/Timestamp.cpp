#include "Timestamp.h"
#include <ctime>

Timestamp::Timestamp()
{
	sesinceepoch_ = time(0);   //获取当前系统的时间
}

Timestamp::Timestamp(int64_t secinceepoch):sesinceepoch_(secinceepoch)
{

}

Timestamp Timestamp::now() {
	return Timestamp();      
	//返回Timestamp 对象。
}

time_t Timestamp::toint() const
{
	return sesinceepoch_;
}

std::string Timestamp::tostring() const
{
	char buf[64] = { 0 };
	tm* tm_time = localtime(&sesinceepoch_);
	snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
		tm_time->tm_year + 1900,
		tm_time->tm_mon + 1,
		tm_time->tm_mday,
		tm_time->tm_hour,
		tm_time->tm_min,
		tm_time->tm_sec);
	return buf;
}
