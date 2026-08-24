#include "Server.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

Server::Server(int port, const std::string& password)
	: _listenFd(-1), _port(port), _password(password), _running(false) {}

Server::~Server() {
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		close(it->first);
	if (_listenFd >= 0)
		close(_listenFd);
}

void Server::setNonBlocking(int fd) {
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl failed");
}

void Server::addPollFd(int fd, short events) {
	struct pollfd p;
	p.fd = fd;
	p.events = events;
	p.revents = 0;
	_pollfds.push_back(p);
}

void Server::removePollFd(int fd) {
	for (size_t i = 0; i < _pollfds.size(); ++i) {
		if (_pollfds[i].fd == fd) {
			_pollfds.erase(_pollfds.begin() + i);
			return;
		}
	}
}

void Server::updatePollOutEvent(int fd, bool enable) {
	for (size_t i = 0; i < _pollfds.size(); ++i) {
		if (_pollfds[i].fd == fd) {
			if (enable) _pollfds[i].events |= POLLOUT;
			else _pollfds[i].events &= ~POLLOUT;
			return;
		}
	}
}

void Server::initSocket() {
	_listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenFd < 0)
		throw std::runtime_error("socket failed");

	int yes = 1;
	if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
		throw std::runtime_error("setsockopt failed");

	setNonBlocking(_listenFd);

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(_port);

	if (bind(_listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("bind failed");

	if (listen(_listenFd, SOMAXCONN) < 0)
		throw std::runtime_error("listen failed");

	addPollFd(_listenFd, POLLIN);
	std::cout << "Listening on port " << _port << std::endl;
}

void Server::acceptNewClients() {
	while (true) {
		struct sockaddr_in cliAddr;
		socklen_t len = sizeof(cliAddr);
		int cfd = accept(_listenFd, (struct sockaddr*)&cliAddr, &len);
		if (cfd < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) break;
			std::cerr << "accept error\n";
			break;
		}
		try {
			setNonBlocking(cfd);
			_clients[cfd] = Client(cfd);
			addPollFd(cfd, POLLIN);
			std::cout << "Client connected fd=" << cfd << std::endl;
		} catch (...) {
			close(cfd);
		}
	}
}

void Server::receiveFromClient(int fd) {
	char buffer[512];
	ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
	if (n <= 0) {
		if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
			disconnectClient(fd);
		return;
	}
	Client& c = _clients[fd];
	c.appendToInBuffer(std::string(buffer, n));

	std::string line;
	while (c.popOneLine(line)) {
		std::cout << "[fd " << fd << "] line: " << line << std::endl;
		// temporary echo for transport testing
		c.queueMessage("You said: " + line + "\r\n");
		updatePollOutEvent(fd, true);
	}
}

void Server::sendToClient(int fd) {
	Client& c = _clients[fd];
	std::string& out = c.outBuffer();
	if (out.empty()) {
		updatePollOutEvent(fd, false);
		return;
	}
	ssize_t n = send(fd, out.c_str(), out.size(), 0);
	if (n < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			disconnectClient(fd);
		return;
	}
	out.erase(0, n);
	if (out.empty())
		updatePollOutEvent(fd, false);
}

void Server::disconnectClient(int fd) {
	std::cout << "Client disconnected fd=" << fd << std::endl;
	close(fd);
	_clients.erase(fd);
	removePollFd(fd);
}

void Server::handlePollEvent(size_t i) {
	int fd = _pollfds[i].fd;
	short rev = _pollfds[i].revents;

	if (fd == _listenFd) {
		if (rev & POLLIN) acceptNewClients();
		return;
	}
	if (rev & (POLLERR | POLLHUP | POLLNVAL)) {
		disconnectClient(fd);
		return;
	}
	if (rev & POLLIN) receiveFromClient(fd);
	if (_clients.find(fd) != _clients.end() && (rev & POLLOUT)) sendToClient(fd);
}

void Server::run() {
	_running = true;
	while (_running) {
		int ret = poll(&_pollfds[0], _pollfds.size(), -1);
		if (ret < 0) {
			if (errno == EINTR) continue;
			throw std::runtime_error("poll failed");
		}
		for (size_t i = 0; i < _pollfds.size(); ++i) {
			if (_pollfds[i].revents)
				handlePollEvent(i);
		}
	}
}

void Server::stop() { _running = false; }