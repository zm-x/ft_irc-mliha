#include "../includes/Client.hpp"

Client::Client()
    : _fd(-1),
      _authenticated(false),
      _registered(false)
{
}

Client::Client(int fd)
    : _fd(fd),
      _authenticated(false),
      _registered(false)
{
}

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

// geter//serter for l3ibat
const std::string& Client::getNickname() const
{
    return _nickname;
}

const std::string& Client::getUsername() const
{
    return _username;
}

const std::string& Client::getRealname() const
{
    return _realname;
}

void Client::setNickname(const std::string& nickname)
{
    _nickname = nickname;
}

void Client::setUsername(const std::string& username)
{
    _username = username;
}

void Client::setRealname(const std::string& realname)
{
    _realname = realname;
}

bool Client::isAuthenticated() const
{
    return _authenticated;
}

bool Client::isRegistered() const
{
    return _registered;
}

void Client::setAuthenticated(bool value)
{
    _authenticated = value;
}

void Client::setRegistered(bool value)
{
    _registered = value;
}