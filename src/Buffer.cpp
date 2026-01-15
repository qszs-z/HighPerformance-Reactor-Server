#include "Buffer.h"
#include <cstring>
Buffer::Buffer(uint16_t sep):sep_(sep){

}

Buffer::~Buffer() {

}

void Buffer::append(const char* data, size_t size) {
	buf_.append(data, size);
}
void Buffer::appendwithsep(const char* data, size_t size)
{
	if (sep_ == 0) {
		buf_.append(data, size);     //不需要处理报头直接 发数据
	}
	else if (sep_ == 1) {          //四字节报头
		buf_.append((char*)&size, 4);
		buf_.append(data, size);
	}

}
void Buffer::erase(size_t pos, size_t nn)
{
	buf_.erase(pos, nn);
}
size_t Buffer::size() {
	return buf_.size();
}

const char* Buffer::data() {
	return buf_.data();
}

void Buffer::clear() {
	buf_.clear();
}
//在Connection::onmessage中调用
bool Buffer::pickmesssage(std::string& ss) {
	if (buf_.size () == 0) return false;

	if (sep_ == 0) {
		ss = buf_;
		buf_.clear();
	}
	else if (sep_ == 1) {
		int len;
		memcpy(&len, buf_.data(), 4);
		if (buf_.size() < len + 4) return false;
		//5abcde     取4个字节   包头[0x00 0x00 0x00 0x05] (4 bytes)   数据[a] [b] [c] [d] [e] (5 bytes)
		//8abcdefeh  
		//6 abc   小于6 加 4  不完整跳出  留在接受缓冲区中
		ss = buf_.substr(4, len); //获取一个报文
		buf_.erase(0, len + 4);  //  删除刚刚读取的报文
	}
	return true;
}