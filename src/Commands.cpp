#include "../includes/Server.hpp"

int Server::PASS_command(Client &c, std::string &param, int fd)
{
	if (param.empty())
	{
		c.queueMessage(":server 461 PASS :Not enough parameters\r\n");
		updatePollOutEvent(fd, true);
		return 1;
	}
	if (param == _password)
		c.setPasswordAccepted(true);
	else
	{
		c.setPasswordAccepted(false);
		c.queueMessage(":server 464 :Password incorrect\r\n");
		updatePollOutEvent(fd, true);
		return 0;
	}
	return 1;
}

int Server::NICK_command(Client &c, std::string &param, int fd)
{
	if (param.empty())
	{
		c.queueMessage(":server 431 NICK :No nickname given\r\n");
		updatePollOutEvent(fd, true);
		return 1;
	}
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
   		if (it->first != fd && it->second.getNickname() == param)
		{
       		c.queueMessage(":server 433 * " + param + " :Nickname is already in use\r\n");
       		updatePollOutEvent(fd, true);
       		return 1;
   		}
	}
	c.setNickname(param);
	return 1;
}

int Server::PING_command(Client &c, std::string &param, int fd)
{
	if (param.empty()) 
	{
    	c.queueMessage(":server 409 :No origin specified\r\n");
    	updatePollOutEvent(fd, true);
    	return 1;
   	}
    c.queueMessage("PONG " + param + "\r\n");
    updatePollOutEvent(fd, true);
    return 1;
}
