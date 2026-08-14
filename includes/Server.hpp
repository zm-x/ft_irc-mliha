#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include "Client.hpp"

class Server {
private:
	int _listenFd;
	int _port;
	std::string _password;
	bool _running;

	std::vector<struct pollfd> _pollfds;
	std::map<int, Client> _clients;

public:
	Server(int port, const std::string& password);
	~Server();

	void initSocket();
	void run();
	void stop();

private:
	void handlePollEvent(size_t i);
	void acceptNewClients();
	void receiveFromClient(int fd);
	void sendToClient(int fd);
	void disconnectClient(int fd);

	void setNonBlocking(int fd);
	void addPollFd(int fd, short events);
	void removePollFd(int fd);
	void updatePollOutEvent(int fd, bool enable);
};

#endif