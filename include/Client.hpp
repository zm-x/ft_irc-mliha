#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>

class Client {
private:
	int         _fd;
	std::string _inBuffer;
	std::string _outBuffer;
	bool        _passwordAccepted;
	std::string _nickname;
public:
	Client();
	Client(int fd);
	~Client();

	int  getFd() const;
	void setFd(int fd);

	void setNickname(const std::string& nick);
	std::string getNickname() const;

	void appendToInBuffer(const std::string& data);
	bool popOneLine(std::string& line); // extracts one line ending with \r\n (without \r\n)

	void queueMessage(const std::string& msg); // appends raw msg to outbuffer
	std::string& outBuffer();

	bool isPasswordAccepted() const;
    void setPasswordAccepted(bool value);
};

#endif