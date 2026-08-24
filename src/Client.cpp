#include "Client.hpp"

Client::Client() : _fd(-1) {}
Client::Client(int fd) : _fd(fd) {}
Client::~Client() {}

int Client::getFd() const { return _fd; }
void Client::setFd(int fd) { _fd = fd; }

void Client::appendToInBuffer(const std::string& data) {
	_inBuffer += data;
}

bool Client::popOneLine(std::string& line) {
	size_t pos = _inBuffer.find("\r\n");
	if (pos == std::string::npos)
		return false;
	line = _inBuffer.substr(0, pos);
	_inBuffer.erase(0, pos + 2);
	return true;
}

void Client::queueMessage(const std::string& msg) {
	_outBuffer += msg;
}

std::string& Client::outBuffer() {
	return _outBuffer;
}