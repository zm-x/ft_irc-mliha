#include "../includes/Server.hpp"
#include "../includes/Client.hpp"


static void checkAndRegisterClient(Server *server, Client &c, int fd)
{
    if (c.isAuthenticated() && !c.getNickname().empty() && !c.getUsername().empty() && !c.isRegistered())
    {
        c.setRegistered(true);

        std::string nick = c.getNickname();
        std::string user = c.getUsername();

        c.queueMessage(":ircserv 001 " + nick + " :Welcome to the Internet Relay Network " + nick + "!" + user + "@localhost\r\n");
        c.queueMessage(":ircserv 002 " + nick + " :Your host is ircserv, running version 1.0\r\n");
        c.queueMessage(":ircserv 003 " + nick + " :This server was created today\r\n");
        c.queueMessage(":ircserv 004 " + nick + " ircserv 1.0 o itkol\r\n");

        server->updatePollOutEvent(fd, true);
    }
}

int Server::PASS_command(Client &c, std::string &param, int fd)
{
    if (c.isRegistered())
    {
        c.queueMessage(":ircserv 462 " + (c.getNickname().empty() ? "*" : c.getNickname()) + " :Unauthorized command (already registered)\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }

    if (param.empty())
    {
        c.queueMessage(":ircserv 461 * PASS :Not enough parameters\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }

    // remove leading ':' from password if present
    if (param[0] == ':')
        param.erase(0, 1);

    if (param == _password)
    {
        c.setPasswordAccepted(true);
    }
    else
    {
        c.setPasswordAccepted(false);
        c.queueMessage(":ircserv 464 * :Password incorrect\r\n");
        updatePollOutEvent(fd, true);
        return 0;
    }
    return 1;
}

int Server::NICK_command(Client &c, std::string &param, int fd)
{
    if (!c.isAuthenticated())
    {
        c.queueMessage(":ircserv 451 * :You have not registered (password required)\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }

    if (param.empty())
    {
        c.queueMessage(":ircserv 431 " + (c.getNickname().empty() ? "*" : c.getNickname()) + " :No nickname given\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }

    // extract the nickname and strip leading spaces or ':'
    std::istringstream iss(param);
    std::string newNick;
    iss >> newNick;
    if (!newNick.empty() && newNick[0] == ':')
        newNick.erase(0, 1);

    // التحقق من تكرار الاسم في السيرفر
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->first != fd && it->second.getNickname() == newNick)
        {
            c.queueMessage(":ircserv 433 " + (c.getNickname().empty() ? "*" : c.getNickname()) + " " + newNick + " :Nickname is already in use\r\n");
            updatePollOutEvent(fd, true);
            return 1;
        }
    }

    // إذا كان العميل مسجلاً مسبقاً ويقوم بتغيير اسمه: بث التغيير
    if (c.isRegistered())
    {
        std::string nickChangeMsg = ":" + c.getNickname() + "!" + c.getUsername() + "@localhost NICK :" + newNick + "\r\n";
        c.queueMessage(nickChangeMsg);
        updatePollOutEvent(fd, true);
    }

    c.setNickname(newNick);

    // التحقق من اكتمال التسجيل (في حال أرسل USER قبل NICK)
    checkAndRegisterClient(this, c, fd);

    return 1;
}

int Server::JOIN_command(Client &c, std::string &param, int fd)
{
    // check if he already registreted
    if (!c.isRegistered())
	{
        c.queueMessage(":ircserv 451 " + (c.getNickname().empty() ? "*" : c.getNickname()) + " :You have not registered\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // check if there is more than "JOIN"
    if (param.empty())
	{
        c.queueMessage(":ircserv 461 " + c.getNickname() + " JOIN :Not enough parameters\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // split key from channel name
    std::string channelName, key;
    size_t spacePos = param.find(' ');
    if (spacePos == std::string::npos)
	{
        channelName = param;
    } else
	{
        channelName = param.substr(0, spacePos);
        key = param.substr(spacePos + 1);
        size_t nextSpace = key.find(' ');
        if (nextSpace != std::string::npos)
            key = key.substr(0, nextSpace);
    }
    // check if there is no "&" or "#" in the channel name
    if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&'))
	{
        c.queueMessage(":ircserv 403 " + c.getNickname() + " " + channelName + " :No such channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    Channel* ch = getChannel(channelName);
    bool isNewChannel = (ch == NULL);
    // create non exit channel
    if (isNewChannel)
	{
        ch = createChannel(channelName);
        if (!ch)
		{
            c.queueMessage(":ircserv 403 " + c.getNickname() + " " + channelName + " :No such channel\r\n");
            updatePollOutEvent(fd, true);
            return 1;
        }
    } else
	{
        // check if the user already memeber in the channel
        if (ch->hasMember(fd))
            return 1;

        //check if the channel for the invited only (i+)
        if (ch->isInviteOnly() && !ch->isInvited(fd))
		{
            c.queueMessage(":ircserv 473 " + c.getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n");
            updatePollOutEvent(fd, true);
            return 1;
        }
        //check if there is password for the channel to join
        if (ch->hasKey() && ch->getKey() != key)
		{
            c.queueMessage(":ircserv 475 " + c.getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n");
            updatePollOutEvent(fd, true);
            return 1;
        }
        // check if the channel reach her limit
        if (ch->hasLimit() && ch->getMembers().size() >= ch->getLimit())
		{
            c.queueMessage(":ircserv 471 " + c.getNickname() + " " + channelName + " :Cannot join channel (+l)\r\n");
            updatePollOutEvent(fd, true);
            return 1;
        }
    }
    // 
    ch->addMember(fd);
    if (ch->isInvited(fd))
        ch->removeInvite(fd);

    //make the user admin if he created the channel
    if (isNewChannel || ch->getMembers().size() == 1)
        ch->addOperator(fd);

    // broadcast the joining message to all channel memebers
    std::string joinMsg = ":" + c.getNickname() + "!" + c.getUsername() + "@localhost JOIN :" + channelName + "\r\n";
    const std::vector<int>& members = ch->getMembers();
    for (size_t i = 0; i < members.size(); ++i)
	{
        int mfd = members[i];
        Client* member = getClient(mfd);
        if (member)
		{
            member->queueMessage(joinMsg);
            updatePollOutEvent(mfd, true);
        }
    }
    // send topic title if exist
    const std::string& curTopic = ch->getTopic();
    if (!curTopic.empty())
	{
        c.queueMessage(":ircserv 332 " + c.getNickname() + " " + channelName + " :" + curTopic + "\r\n");
        updatePollOutEvent(fd, true);
    }
    // show the channel members info to the new member
    std::string namesList;
    for (size_t i = 0; i < members.size(); ++i)
	{
        Client* member = getClient(members[i]);
        if (!member) continue;
        if (!namesList.empty())
            namesList += " ";
        if (ch->isOperator(members[i]))
            namesList += "@" + member->getNickname();
        else
            namesList += member->getNickname();
    }
	//print the result of all memebers list in the channel
    c.queueMessage(":ircserv 353 " + c.getNickname() + " = " + channelName + " :" + namesList + "\r\n");
    c.queueMessage(":ircserv 366 " + c.getNickname() + " " + channelName + " :End of /NAMES list.\r\n");
    updatePollOutEvent(fd, true);
    return 1;
}

int Server::KICK_command(Client &c, std::string &param, int fd)
{
    if (!c.isRegistered())
	{
        c.queueMessage(":ircserv 451 " + (c.getNickname().empty() ? "*" : c.getNickname()) + " :You have not registered\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    if (param.empty())
	{
        c.queueMessage(":ircserv 461 " + c.getNickname() + " KICK :Not enough parameters\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    std::istringstream iss(param);
    std::string channelName, targetNick;
    iss >> channelName >> targetNick;
    if (channelName.empty() || targetNick.empty())
	{
        c.queueMessage(":ircserv 461 " + c.getNickname() + " KICK :Not enough parameters\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }

    //get kick reason
	//default kick reason is name of the unwanted member
    std::string reason = c.getNickname();
    size_t colonPos = param.find(" :");
    if (colonPos != std::string::npos)
        reason = param.substr(colonPos + 2);
	else
	{
        std::string thirdWord;
        if (iss >> thirdWord)
            reason = thirdWord;
    }
    //check if the channel exist
    Channel *ch = getChannel(channelName);
    if (!ch)
	{
        c.queueMessage(":ircserv 403 " + c.getNickname() + " " + channelName + " :No such channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // check if the kicker in the channel
    if (!ch->hasMember(fd))
	{
        c.queueMessage(":ircserv 442 " + c.getNickname() + " " + channelName + " :You're not on that channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // check if the kicker is admin
    if (!ch->isOperator(fd))
	{
        c.queueMessage(":ircserv 482 " + c.getNickname() + " " + channelName + " :You're not channel operator\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // check if the unwanted member in the channel
    Client *targetClient = findClientByNickname(targetNick);
    if (!targetClient || !ch->hasMember(targetClient->getFd()))
	{
        c.queueMessage(":ircserv 441 " + c.getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    //prepar for to broadcast kick message to the channel members including the unwanted member
    std::string kickMsg = ":" + c.getNickname() + "!" + c.getUsername() + "@localhost KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    const std::vector<int> &members = ch->getMembers();
    for (size_t i = 0; i < members.size(); ++i)
	{
        Client *member = getClient(members[i]);
        if (member)
		{
            member->queueMessage(kickMsg);
            updatePollOutEvent(members[i], true);
        }
    }
    // remove the unwanted member
    ch->removeMember(targetClient->getFd());
    // delete channel if it became empty
    if (ch->getMembers().empty())
        _channels.erase(channelName);

    return 1;
}

int Server::INVITE_command(Client &c, std::string &param, int fd)
{
    if (!c.isRegistered())
	{
        c.queueMessage(":ircserv 451 " + (c.getNickname().empty() ? "*" : c.getNickname()) + " :You have not registered\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    if (param.empty())
	{
        c.queueMessage(":ircserv 461 " + c.getNickname() + " INVITE :Not enough parameters\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    std::istringstream iss(param);
    std::string targetNick, channelName;
    iss >> targetNick >> channelName;
    if (targetNick.empty() || channelName.empty())
	{
        c.queueMessage(":ircserv 461 " + c.getNickname() + " INVITE :Not enough parameters\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    //check if the channel exist in the server
    Channel *ch = getChannel(channelName);
    if (!ch)
	{
        c.queueMessage(":ircserv 403 " + c.getNickname() + " " + channelName + " :No such channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // check if the inviter is channel member
    if (!ch->hasMember(fd))
	{
        c.queueMessage(":ircserv 442 " + c.getNickname() + " " + channelName + " :You're not on that channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // check if the inviter is admin
    if (ch->isInviteOnly() && !ch->isOperator(fd))
	{
        c.queueMessage(":ircserv 482 " + c.getNickname() + " " + channelName + " :You're not channel operator\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // check if the invited user is not already in the server
    Client *targetClient = findClientByNickname(targetNick);
    if (!targetClient)
	{
        c.queueMessage(":ircserv 401 " + c.getNickname() + " " + targetNick + " :No such nick/channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // check if the invited user is already a member in the channel
    if (ch->hasMember(targetClient->getFd()))
	{
        c.queueMessage(":ircserv 443 " + c.getNickname() + " " + targetNick + " " + channelName + " :is already on channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    //make the user invite state true
    ch->addInvite(targetClient->getFd());

    // send confirmation message to the member
    c.queueMessage(":ircserv 341 " + c.getNickname() + " " + targetNick + " " + channelName + "\r\n");
    updatePollOutEvent(fd, true);

    // send invitation message to the user :)
    std::string inviteNotice = ":" + c.getNickname() + "!" + c.getUsername() + "@localhost INVITE " + targetNick + " :" + channelName + "\r\n";
    targetClient->queueMessage(inviteNotice);
    updatePollOutEvent(targetClient->getFd(), true);

    return 1;
}

int Server::MODE_command(Client &c, std::string &param, int fd)
{
    if (!c.isRegistered())
	{
        c.queueMessage(":ircserv 451 " + (c.getNickname().empty() ? "*" : c.getNickname()) + " :You have not registered\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    if (param.empty())
	{
        c.queueMessage(":ircserv 461 " + c.getNickname() + " MODE :Not enough parameters\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    std::istringstream iss(param);
    std::string target, modeStr;
    iss >> target >> modeStr;
    if (target[0] != '#' && target[0] != '&')
        return 1;

    //check if the channel exist
    Channel *ch = getChannel(target);
    if (!ch)
	{
        c.queueMessage(":ircserv 403 " + c.getNickname() + " " + target + " :No such channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // write channel mods if there is no mods set in the argument
    if (modeStr.empty())
	{
        std::string currentModes = "+";
        if (ch->isInviteOnly()) currentModes += "i";
        if (ch->isTopicRestricted()) currentModes += "t";
        if (ch->hasKey()) currentModes += "k";
        if (ch->hasLimit()) currentModes += "l";
        if (currentModes == "+") currentModes = "";
        c.queueMessage(":ircserv 324 " + c.getNickname() + " " + target + " " + currentModes + "\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    //check if the member is admin
    if (!ch->isOperator(fd))
	{
        c.queueMessage(":ircserv 482 " + c.getNickname() + " " + target + " :You're not channel operator\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // apply requested mods
    bool adding = true;
    std::string appliedModes = "";
    std::string appliedArgs = "";
    for (size_t i = 0; i < modeStr.size(); ++i)
	{
        char m = modeStr[i];

        if (m == '+')
		{
            adding = true;
            continue;
        }
        if (m == '-')
		{
            adding = false;
            continue;
        }

        //apply i
        if (m == 'i')
		{
            ch->setInviteOnly(adding);
            appliedModes += (adding ? "+i" : "-i");
        }
        //apply t
        else if (m == 't')
		{
            ch->setTopicRestricted(adding);
            appliedModes += (adding ? "+t" : "-t");
        }
        // apply k
        else if (m == 'k')
		{
            if (adding)
			{
                std::string keyArg;
                if (iss >> keyArg)
				{
                    ch->setKey(keyArg);
                    appliedModes += "+k";
                    appliedArgs += " " + keyArg;
                } else
				{
                    c.queueMessage(":ircserv 461 " + c.getNickname() + " MODE :Not enough parameters\r\n");
                    updatePollOutEvent(fd, true);
                }
            } 
			else
			{
                ch->setKey("");
                appliedModes += "-k";
            }
        }
        // apply o
        else if (m == 'o')
		{
            std::string userArg;
            if (iss >> userArg)
			{
                Client *targetUser = findClientByNickname(userArg);
                if (!targetUser || !ch->hasMember(targetUser->getFd()))
				{
                    c.queueMessage(":ircserv 441 " + c.getNickname() + " " + userArg + " " + target + " :They aren't on that channel\r\n");
                    updatePollOutEvent(fd, true);
                }
				else
				{
                    if (adding)
                        ch->addOperator(targetUser->getFd());
                    else
                        ch->removeOperator(targetUser->getFd());
                    appliedModes += (adding ? "+o" : "-o");
                    appliedArgs += " " + userArg;
                }
            }
			else
			{
                c.queueMessage(":ircserv 461 " + c.getNickname() + " MODE :Not enough parameters\r\n");
                updatePollOutEvent(fd, true);
            }
        }
        //apply p
        else if (m == 'l')
		{
            if (adding)
			{
                std::string limitArg;
                if (iss >> limitArg)
				{
                    int lim = std::atoi(limitArg.c_str());
                    if (lim > 0)
					{
                        ch->setLimit(static_cast<size_t>(lim));
                        appliedModes += "+l";
                        appliedArgs += " " + limitArg;
                    }
                }
				else
				{
                    c.queueMessage(":ircserv 461 " + c.getNickname() + " MODE :Not enough parameters\r\n");
                    updatePollOutEvent(fd, true);
                }
            }
			else
			{
                ch->setLimit(0);
                appliedModes += "-l";
            }
        }
        // undefined character
        else
		{
            c.queueMessage(":ircserv 472 " + c.getNickname() + " " + m + " :is unknown mode char to me\r\n");
            updatePollOutEvent(fd, true);
        }
    }
    //broadcast changes to all memebers
    if (!appliedModes.empty())
	{
        std::string modeNotice = ":" + c.getNickname() + "!" + c.getUsername() + "@localhost MODE " + target + " " + appliedModes + appliedArgs + "\r\n";
        const std::vector<int> &members = ch->getMembers();
        for (size_t i = 0; i < members.size(); ++i)
		{
            Client *member = getClient(members[i]);
            if (member)
			{
                member->queueMessage(modeNotice);
                updatePollOutEvent(members[i], true);
            }
        }
    }

    return 1;
}

int Server::QUIT_command(Client &c, std::string &param, int fd)
{
    // get quit reason
    std::string reason = "Client Quit";
    if (!param.empty())
	{
        reason = param;
        if (reason[0] == ':')
            reason.erase(0, 1);
    }
    std::string quitMsg = ":" + c.getNickname() + "!" + c.getUsername() + "@localhost QUIT :" + reason + "\r\n";
    // get all members from all channel he already in it
    std::set<int> sharedMembers;
    std::vector<std::string> emptyChannels;
    for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
        Channel &ch = it->second;
        if (ch.hasMember(fd))
		{
            // stack all members in one container
            const std::vector<int> &m = ch.getMembers();
            for (size_t i = 0; i < m.size(); ++i)
			{
                if (m[i] != fd)
                    sharedMembers.insert(m[i]);
            }
            // remove user from channel
            ch.removeMember(fd);

            // check if the channel is empty
            if (ch.getMembers().empty())
                emptyChannels.push_back(it->first);
        }
    }
    // send message to all memebers
    for (std::set<int>::iterator it = sharedMembers.begin(); it != sharedMembers.end(); ++it)
	{
        Client *member = getClient(*it);
        if (member)
		{
            member->queueMessage(quitMsg);
            updatePollOutEvent(*it, true);
        }
    }
    // free memory from empty channels
    for (size_t i = 0; i < emptyChannels.size(); ++i)
	{
        removeChannel(emptyChannels[i]);
    }
    //remove user from exist :)
    disconnectClient(fd);
    return 1;
}

#include <sstream>

int Server::PART_command(Client &c, std::string &param, int fd)
{
    if (!c.isRegistered()) {
        c.queueMessage(":ircserv 451 " + (c.getNickname().empty() ? "*" : c.getNickname()) + " :You have not registered\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    if (param.empty()) {
        c.queueMessage(":ircserv 461 " + c.getNickname() + " PART :Not enough parameters\r\n");
        updatePollOutEvent(fd, true);
        return 1;
	}
    std::string channelName;
    std::string reason = "Leaving";
    size_t spacePos = param.find(' ');
    if (spacePos == std::string::npos) {
        channelName = param;
    } else {
        channelName = param.substr(0, spacePos);
        std::string rawReason = param.substr(spacePos + 1);
        if (!rawReason.empty()) {
            if (rawReason[0] == ':')
                rawReason.erase(0, 1);
            if (!rawReason.empty())
                reason = rawReason;
        }
    }

    // check if the channel exist
    Channel *ch = getChannel(channelName);
    if (!ch) {
        c.queueMessage(":ircserv 403 " + c.getNickname() + " " + channelName + " :No such channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    // check if the user is member
    if (!ch->hasMember(fd)) {
        c.queueMessage(":ircserv 442 " + c.getNickname() + " " + channelName + " :You're not on that channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }

    // broadcast the leaving message
    std::string partMsg = ":" + c.getNickname() + "!" + c.getUsername() + "@localhost PART " + channelName + " :" + reason + "\r\n";
    const std::vector<int> &members = ch->getMembers();
    for (size_t i = 0; i < members.size(); ++i) {
        Client *member = getClient(members[i]);
        if (member) {
            member->queueMessage(partMsg);
            updatePollOutEvent(members[i], true);
        }
    }
    // remove the member from the channel
    ch->removeMember(fd);
    // delete channel if it bacame empty
    if (ch->getMembers().empty()) {
        _channels.erase(channelName);
    }
    return 1;
}

int Server::PRIVMSG_command(Client &c, std::string &param, int fd)
{
    if (!c.isRegistered())
	{
        c.queueMessage(":ircserv 451 " + (c.getNickname().empty() ? "*" : c.getNickname()) + " :You have not registered\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    if (param.empty())
	{
        c.queueMessage(":ircserv 411 " + c.getNickname() + " :No recipient given (PRIVMSG)\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    size_t spacePos = param.find(' ');
    if (spacePos == std::string::npos)
	{
        c.queueMessage(":ircserv 412 " + c.getNickname() + " :No text to send\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    std::string target = param.substr(0, spacePos);
    std::string message = param.substr(spacePos + 1);
    // remove ": " from the message
    if (!message.empty() && message[0] == ':')
        message.erase(0, 1);
    if (message.empty())
	{
        c.queueMessage(":ircserv 412 " + c.getNickname() + " :No text to send\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    std::string formattedMsg = ":" + c.getNickname() + "!" + c.getUsername() + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
    // check if the message sent to channel
    if (target[0] == '#' || target[0] == '&')
	{
        Channel *ch = getChannel(target);
		//check if the channel exist
        if (!ch)
		{
            c.queueMessage(":ircserv 403 " + c.getNickname() + " " + target + " :No such channel\r\n");
            updatePollOutEvent(fd, true);
            return 1;
		}
		//check if the user is memeber
        if (!ch->hasMember(fd))
		{
            c.queueMessage(":ircserv 404 " + c.getNickname() + " " + target + " :Cannot send to channel\r\n");
            updatePollOutEvent(fd, true);
            return 1;
        }
        const std::vector<int> &members = ch->getMembers();
        for (size_t i = 0; i < members.size(); ++i)
		{
            int mfd = members[i];
            if (mfd != fd)
			{
            	Client *member = getClient(mfd);
                if (member)
				{
                    member->queueMessage(formattedMsg);
                    updatePollOutEvent(mfd, true);
                }
            }
        }
    }
    // direct message
    else
	{
        Client *targetClient = findClientByNickname(target);
        if (!targetClient)
		{
            c.queueMessage(":ircserv 401 " + c.getNickname() + " " + target + " :No such nick/channel\r\n");
            updatePollOutEvent(fd, true);
            return 1;
        }
        targetClient->queueMessage(formattedMsg);
        updatePollOutEvent(targetClient->getFd(), true);
    }

    return 1;
}

int Server::TOPIC_command(Client &c, std::string &param, int fd)
{
    if (param.empty())
	{
        c.queueMessage(":server 461 " + c.getNickname() + " TOPIC :Not enough parameters\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    size_t spacePos = param.find(' ');
	//check if there is topic
    std::string channel = (spacePos == std::string::npos) ? param : param.substr(0, spacePos);
    bool isSetting = (spacePos != std::string::npos);
    std::string topic = "";
	//remove ":" from the topic if there
    if (isSetting)
	{
        topic = param.substr(spacePos + 1);
        if (!topic.empty() && topic[0] == ':')
            topic.erase(0, 1);
    }
    Channel* ch = getChannel(channel);
	//check if there is channel contain same name
    if (!ch)
	{
        c.queueMessage(":server 403 " + c.getNickname() + " " + channel + " :No such channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
	//check if the the current user is member in the channel
    if (!ch->hasMember(fd))
	{
        c.queueMessage(":server 442 " + c.getNickname() + " " + channel + " :You're not on that channel\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
	//check the topic of the channel
    if (!isSetting)
	{
        const std::string& cur = ch->getTopic();
        if (cur.empty())
            c.queueMessage(":server 331 " + c.getNickname() + " " + channel + " :No topic is set\r\n");
        else
            c.queueMessage(":server 332 " + c.getNickname() + " " + channel + " :" + cur + "\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
	//check if u can change the topic
    if (ch->isTopicRestricted() && !ch->isOperator(fd))
	{
        c.queueMessage(":server 482 " + c.getNickname() + " " + channel + " :You're not channel operator\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    ch->setTopic(topic);
    std::string msg = ":" + c.getNickname() + "!" + c.getUsername() + "@localhost TOPIC " + channel + " :" + topic + "\r\n";
    const std::vector<int>& members = ch->getMembers();
	//send msg about the changes to all members
    for (size_t i = 0; i < members.size(); ++i)
	{
        int mfd = members[i];
        Client* member = getClient(mfd);
        if (member)
		{
            member->queueMessage(msg);
            updatePollOutEvent(mfd, true);
        }
    }
    return 1;
}

int Server::USER_command(Client &c, std::string &param, int fd)
{
    if (!c.isAuthenticated())
    {
        c.queueMessage(":server 451 * :You have not registered (password required)\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }
    if (c.isRegistered())
    {
        c.queueMessage(":server 462 " + c.getNickname() + " :Unauthorized command (already registered)\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }

    std::istringstream iss(param);
    std::string username, mode, unused;
    iss >> username >> mode >> unused;

    // find ':' anywhere after the three parameters
    std::string realname = "";
    size_t colonPos = param.find(':');
    if (colonPos != std::string::npos) {
        realname = param.substr(colonPos + 1);
    } else {
        iss >> realname;
    }

    if (username.empty() || mode.empty() || unused.empty() || realname.empty())
    {
        c.queueMessage(":server 461 " + (c.getNickname().empty() ? "*" : c.getNickname()) + " USER :Not enough parameters\r\n");
        updatePollOutEvent(fd, true);
        return 1;
    }

    c.setUsername(username);
    c.setRealname(realname);

    // complete registration if NICK was already set
    if (!c.getNickname().empty() && !c.isRegistered())
    {
        c.setRegistered(true);
        std::string welcome = ":server 001 " + c.getNickname() + " :Welcome to the IRC Network " 
                            + c.getNickname() + "!" + c.getUsername() + "@localhost\r\n";
        c.queueMessage(welcome);
        updatePollOutEvent(fd, true);
    }
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
