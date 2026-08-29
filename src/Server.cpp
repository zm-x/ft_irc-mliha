#include "../includes/Server.hpp"

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

void Server::receiveFromClient(int fd)
{
    char buffer[512];
    ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
    if (n <= 0)
    {
        if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
            disconnectClient(fd);
        return;
    }
    Client& c = _clients[fd];
    c.appendToInBuffer(std::string(buffer, n));

    std::string line;
    while (c.popOneLine(line)) 
    {
        if (line.empty())
            continue;

        size_t spacePos = line.find(' ');
        std::string cmd;
        std::string param = "";

        if (spacePos != std::string::npos)
        {
            cmd = line.substr(0, spacePos);
			// skip extra spaces between command and parameters
			size_t paramStart = line.find_first_not_of(' ', spacePos);
            if (paramStart != std::string::npos)
                param = line.substr(paramStart);
        }
        else
        {
            cmd = line;
        }

        if (cmd.empty())
            continue;

        int return_value = 1;

        if (cmd == "PASS")
            return_value = PASS_command(c, param, fd);
        else if (cmd == "NICK")
            return_value = NICK_command(c, param, fd);
        else if (cmd == "USER")
            return_value = USER_command(c, param, fd);
        else if (cmd == "PING")
            return_value = PING_command(c, param, fd);
        else if (cmd == "JOIN")
            return_value = JOIN_command(c, param, fd);
        else if (cmd == "PRIVMSG")
            return_value = PRIVMSG_command(c, param, fd);
        else if (cmd == "QUIT")
            return_value = QUIT_command(c, param, fd);
        else if (cmd == "KICK")
            return_value = KICK_command(c, param, fd);
        else if (cmd == "MODE")
            return_value = MODE_command(c, param, fd);
        else if (cmd == "TOPIC")
            return_value = TOPIC_command(c, param, fd);
        else if (cmd == "INVITE")
            return_value = INVITE_command(c, param, fd);
        else if (cmd == "PART")
            return_value = PART_command(c, param, fd);
		else
		{
			c.queueMessage(":ircserv 421 " + c.getNickname() + " " + cmd + " :Unknown command\r\n");
			updatePollOutEvent(fd, true);
		}

        if (!return_value)
        {
            disconnectClient(fd);
            return;
        }
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


/// server hellper for channels

Client* Server::getClient(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);

    if (it == _clients.end())
        return NULL;

    return &it->second;
}

Client* Server::findClientByNickname(const std::string& nickname)
{
    for (std::map<int, Client>::iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        if (it->second.getNickname() == nickname)
            return &it->second;
    }

    return NULL;
}

bool Server::nicknameExists(const std::string& nickname) const
{
    for (std::map<int, Client>::const_iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        if (it->second.getNickname() == nickname)
            return true;
    }

    return false;
}

Channel* Server::getChannel(const std::string& name)
{
    std::map<std::string, Channel>::iterator it = _channels.find(name);

    if (it == _channels.end())
        return NULL;

    return &it->second;
}

Channel* Server::createChannel(const std::string& name)
{
    if (_channels.find(name) != _channels.end())
        return NULL;

    _channels.insert(std::make_pair(name, Channel(name)));

    return &_channels.find(name)->second;
}

void Server::removeChannel(const std::string& name)
{
    std::map<std::string, Channel>::iterator it = _channels.find(name);

    if (it != _channels.end())
        _channels.erase(it);
}
