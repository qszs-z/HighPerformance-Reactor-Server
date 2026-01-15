#pragma once
#include <string>
#include <iostream>
#include <cstdint>

class Buffer {
private:
	std::string buf_; //string 既可以存放二进制也可以存放文本数据
	const uint16_t sep_;    //报文的分隔符 0-无分隔符（固定长度、视频会议）1-四字节的报头 2-“\r\n\r\n”(http协议)
public:
	Buffer(uint16_t sep = 1);
	~Buffer();

	void append(const char* data, size_t size);         //把数据追加到buffer中
	void appendwithsep(const char* data, size_t size);    //把数据追加到buffer中附加报文头部
	void erase(size_t pos, size_t nn);   //删除的位置  删除的字节
	size_t size();
	const char* data(); //返回首地址
	void clear();    //清空buffer
	bool pickmesssage(std::string& ss);          //从buf_中拆分出一个报文，存放在ss中，如果buf_中没有报文，返回false


};