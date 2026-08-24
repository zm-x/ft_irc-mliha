#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>

class Client {
private:
	int         _fd;
	std::string _inBuffer;
	std::string _outBuffer;

public:
	Client();
	Client(int fd);
	~Client();

	int  getFd() const;
	void setFd(int fd);

	void appendToInBuffer(const std::string& data);
	bool popOneLine(std::string& line); // extracts one line ending with \r\n (without \r\n)

	void queueMessage(const std::string& msg); // appends raw msg to outbuffer
	std::string& outBuffer();
};

#endif