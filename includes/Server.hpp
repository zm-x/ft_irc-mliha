#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <sstream>
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
#include <set>
#include "Client.hpp"
#include "Channel.hpp"

class Server
{
private:
    int _listenFd;
    int _port;
    std::string _password;
    bool _running;

    std::vector<struct pollfd> _pollfds;
    std::map<int, Client> _clients;
    std::map<std::string, Channel> _channels;

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

public:
    Client* getClient(int fd);
    Client* findClientByNickname(const std::string& nickname);
    bool nicknameExists(const std::string& nickname) const;

    Channel* getChannel(const std::string& name);
    Channel* createChannel(const std::string& name);
    void removeChannel(const std::string& name);
    
    int PASS_command(Client &c, std::string &param, int fd);
	int NICK_command(Client &c, std::string &param, int fd);
	int PING_command(Client &c, std::string &param, int fd);
    int TOPIC_command(Client &c, std::string &param, int fd);
    int USER_command(Client &c, std::string &param, int fd);
    int JOIN_command(Client &c, std::string &param, int fd);
    int PRIVMSG_command(Client &c, std::string &param, int fd);
    int QUIT_command(Client &c, std::string &param, int fd);
    int KICK_command(Client &c, std::string &param, int fd);
    int MODE_command(Client &c, std::string &param, int fd);
    int INVITE_command(Client &c, std::string &param, int fd);
    int PART_command(Client &c, std::string &param, int fd);
};

#endif