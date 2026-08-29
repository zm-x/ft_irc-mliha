#include "../includes/Channel.hpp"

Channel::Channel()
    : _name(""), _topic(""), _inviteOnly(false), _topicRestricted(false), _key(""), _limit(0)
{
}

Channel::Channel(const std::string& name)
    : _name(name), _topic(""), _inviteOnly(false), _topicRestricted(false), _key(""), _limit(0)
{
}

Channel::~Channel()
{
}

const std::string& Channel::getName() const
{
    return _name;
}

const std::string& Channel::getTopic() const
{
    return _topic;
}

void Channel::setTopic(const std::string& topic)
{
    _topic = topic;
}

bool Channel::isTopicRestricted() const
{
    return _topicRestricted;
}

void Channel::setTopicRestricted(bool val)
{
    _topicRestricted = val;
}

bool Channel::isInviteOnly() const
{
    return _inviteOnly;
}

void Channel::setInviteOnly(bool val)
{
    _inviteOnly = val;
}

bool Channel::isInvited(int fd) const
{
    for (std::vector<int>::const_iterator it = _invited.begin(); it != _invited.end(); ++it)
    {
        if (*it == fd)
            return true;
    }
    return false;
}

void Channel::addInvite(int fd)
{
    if (!isInvited(fd))
        _invited.push_back(fd);
}

void Channel::removeInvite(int fd)
{
    for (std::vector<int>::iterator it = _invited.begin(); it != _invited.end(); ++it)
    {
        if (*it == fd)
        {
            _invited.erase(it);
            break;
        }
    }
}

bool Channel::hasKey() const
{
    return !_key.empty();
}

const std::string& Channel::getKey() const
{
    return _key;
}

void Channel::setKey(const std::string& key)
{
    _key = key;
}

bool Channel::hasLimit() const
{
    return _limit > 0;
}

size_t Channel::getLimit() const
{
    return _limit;
}

void Channel::setLimit(size_t limit)
{
    _limit = limit;
}

void Channel::addMember(int fd)
{
    if (!hasMember(fd))
        _members.push_back(fd);
}

void Channel::removeMember(int fd)
{
    for (std::vector<int>::iterator it = _members.begin();
         it != _members.end(); ++it)
    {
        if (*it == fd)
        {
            _members.erase(it);
            break;
        }
    }

    removeOperator(fd);
}

bool Channel::hasMember(int fd) const
{
    for (std::vector<int>::const_iterator it = _members.begin();
         it != _members.end(); ++it)
    {
        if (*it == fd)
            return true;
    }

    return false;
}

void Channel::addOperator(int fd)
{
    if (!isOperator(fd))
        _operators.push_back(fd);
}

void Channel::removeOperator(int fd)
{
    for (std::vector<int>::iterator it = _operators.begin();
         it != _operators.end(); ++it)
    {
        if (*it == fd)
        {
            _operators.erase(it);
            break;
        }
    }
}

bool Channel::isOperator(int fd) const
{
    for (std::vector<int>::const_iterator it = _operators.begin();
         it != _operators.end(); ++it)
    {
        if (*it == fd)
            return true;
    }

    return false;
}

const std::vector<int>& Channel::getMembers() const
{
    return _members;
}

const std::vector<int>& Channel::getOperators() const
{
    return _operators;
}
