#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
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
    //cpmmands
    int PASS_command(Client &c, std::string &param, int fd);
	int NICK_command(Client &c, std::string &param, int fd);
	int PING_command(Client &c, std::string &param, int fd);
};

#endif