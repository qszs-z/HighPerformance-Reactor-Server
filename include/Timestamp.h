#pragma once
#include <iostream>
#include <string>

class Timestamp {
private: 
	time_t sesinceepoch_;    //整数表示时间
public:
	Timestamp();
	Timestamp(int64_t secinceepoch);

	static Timestamp now();       //返回当前的时间对象

	time_t toint() const;               //返回整数表示时间

	std::string tostring() const; //返回字符串表示的时间

};