// 网上银行业务的客户端程序
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

// 发送报文，支持4字节的报头。
ssize_t tcpsend(int fd, void* data, size_t size)
{
    char tmpbuf[1024];                    // 临时的buffer，报文头部+报文内容。
    memset(tmpbuf, 0, sizeof(tmpbuf));
    memcpy(tmpbuf, &size, 4);         // 拼接报文头部。
    memcpy(tmpbuf + 4, data, size);  // 拼接报文内容。

    return send(fd, tmpbuf, size + 4, 0);        // 把请求报文发送给服务端。
}

// 接收报文，支持4字节的报头。
ssize_t tcprecv(int fd, void* data)
{
    int len;
    recv(fd, &len, 4, 0);            // 先读取4字节的报文头部。
    return recv(fd, data, len, 0);           // 读取报文内容。
}

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

    printf("connect ok.\n");

    //////////////////////////////////////////
    // 登录业务。
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "<bizcode>00101</bizcode><username>zqz</username><password>123456</password>");
    if (tcpsend(sockfd, buf, strlen((buf))) <= 0) { printf("tcpsend() failed.\n"); return -1; }
    printf("发送：%s\n", buf);

    memset(buf, 0, sizeof(buf));
    if (tcprecv(sockfd, buf) <= 0) { printf("tcprecv() failed.\n"); return -1; }
    printf("接收：%s\n", buf);
    //////////////////////////////////////////

    ////////////////////////////////////////////
    // 查询余额业务。
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "<bizcode>00201</bizcode><cardid>1234567890</cardid>");
    if (tcpsend(sockfd, buf, strlen((buf))) <= 0) { printf("tcpsend() failed.\n"); return -1; }
    printf("发送：%s\n", buf);

    memset(buf, 0, sizeof(buf));
    if (tcprecv(sockfd, buf) <= 0) { printf("tcprecv() failed.\n"); return -1; }
    printf("接收：%s\n", buf);
    //sleep(21);  客户端不想超时就要设置心跳  发送心跳报文
    ////////////////////////////////////////////

    //////////////////////////////////////////
    // 心跳。
    while (true)
    {
        memset(buf,0,sizeof(buf));
        sprintf(buf,"<bizcode>00001</bizcode>");
        if (tcpsend(sockfd,buf,strlen((buf))) <=0) { printf("tcpsend() failed.\n"); return -1; }
        printf("发送：%s\n",buf);

        memset(buf,0,sizeof(buf));
        if (tcprecv(sockfd,buf) <=0) { printf("tcprecv() failed.\n"); return -1; }
        printf("接收：%s\n",buf);

        sleep(5);
    }
    //////////////////////////////////////////


//////////////////////////////////////////
    // 注销业务。
       memset(buf, 0, sizeof(buf));
       sprintf(buf, "<bizcode>00901</bizcode>");
        if (tcpsend(sockfd, buf, strlen((buf))) <= 0) { printf("tcpsend() failed.\n"); return -1; }
       printf("发送：%s\n", buf);

        memset(buf, 0, sizeof(buf));
        if (tcprecv(sockfd, buf) <= 0) { printf("tcprecv() failed.\n"); return -1; }
        printf("接收：%s\n", buf);
    //////////////////////////////////////////
}

