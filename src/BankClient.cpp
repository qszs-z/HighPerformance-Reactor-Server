// 网络通讯的客户端程序。
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctime>
#include <iostream>
//加C字母是(C++ 风格)
int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		printf("usage:./TcpClient ip port\n");
		printf("example:./TcpClient 127.0.0.1 8152\n\n");
		return -1;
	}

	int sockfd;
	struct sockaddr_in servaddr;
	char buf[1024];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { printf("socket() failed.\n"); return -1; }

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);

	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0)
	{
		printf("connect(%s:%s) failed.\n", argv[1], argv[2]); close(sockfd);  return -1;
	}
	std::cout << "connection ok" << std::endl;
	std::cout << "开始时间：" << time(nullptr) << std::endl;
	//报文头部
	
	for (int ii = 0; ii < 100000; ii++)
	{
/// 从命令行输入内容。
		memset(buf, 0, sizeof(buf));
		//printf("please input:"); scanf("%s", buf);
		sprintf(buf, "这是第%d个妮巧。",ii);
		char tmpbuf[1024];
		memset(tmpbuf, 0, sizeof(tmpbuf));
		int len = strlen(buf);
		memcpy(tmpbuf, &len, 4);
		memcpy(tmpbuf + 4, &buf, len);

		send(sockfd, tmpbuf, len + 4, 0);      // 把命令行输入的内容发送给服务端。

		recv(sockfd, &len, 4,0);   //先读取报文头部
		memset(buf, 0, sizeof(buf));
		recv(sockfd, buf, len, 0);    //再读报文剩下的 // 接收服务端的回应。
		//printf("recv:%s\n", buf);

	}
	std::cout << "结束时间：" << time(nullptr) << std::endl;


	//无报文头部的
	/*for (int i = 0; i < 10; i++) {
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "这是第%d个妮巧。", i);
		send(sockfd, buf, strlen(buf), 0);
		memset(buf, 0, sizeof(buf));
		recv(sockfd, buf, 1024, 0);
		printf("recv:%s\n", buf);
		sleep(1);
	}*/
}