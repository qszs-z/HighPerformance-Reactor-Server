#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <string>

//socket 的地址协议类
class InetAddress
{
public:
	InetAddress();
	InetAddress(const std::string& ip, uint16_t port);
	InetAddress(const sockaddr_in addr);
	~InetAddress();

	std::string ip() const;
	uint16_t port() const;
	const sockaddr* addr() const;
	void  setaddr(sockaddr_in clientaddr);
private:
	sockaddr_in addr_;         //表示地址协议的结构体
};

