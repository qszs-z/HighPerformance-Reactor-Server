#include "InetAddress.h"
InetAddress::InetAddress()
{
}
InetAddress::InetAddress(const std::string& ip, uint16_t port) {  // 服务端用于监听的
	memset(&addr_, 0, sizeof(addr_));
	addr_.sin_family = AF_INET;
	inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
	addr_.sin_port = htons(port);
}
InetAddress::InetAddress(const sockaddr_in addr):addr_(addr){ //客户端连接上了fd 就创建一个地址协议的结构体

}
InetAddress::~InetAddress() {

}	
std::string InetAddress::ip() const {   //返回字符串表示
	//return inet_ntoa(addr_.sin_addr);//非线程安全的
	char buffer[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &addr_.sin_addr, buffer, sizeof(buffer));

	return std::string(buffer);
} 
uint16_t InetAddress::port() const {  //返回端口
	return ntohs(addr_.sin_port);
}
const sockaddr* InetAddress::addr() const {  //返回addr_成员的地址，转换成了sockaddr
	return (sockaddr*)&addr_;
}

void InetAddress::setaddr(sockaddr_in clientaddr)
{
	addr_ = clientaddr;
}

