#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>

class Channel
{
private:
    std::string _name;
    std::string _topic;
    std::vector<int> _members;
    std::vector<int> _operators;
    std::vector<int> _invited;
    bool        _inviteOnly;
    bool        _topicRestricted;
    std::string _key;
    size_t      _limit;

public:
    Channel();
    Channel(const std::string& name);
    ~Channel();

    const std::string& getName() const;

    const std::string& getTopic() const;
    void setTopic(const std::string& topic);
    bool isTopicRestricted() const;
    void setTopicRestricted(bool val);

    void addMember(int fd);
    void removeMember(int fd);
    bool hasMember(int fd) const;

    void addOperator(int fd);
    void removeOperator(int fd);
    bool isOperator(int fd) const;

    bool isInviteOnly() const;
    void setInviteOnly(bool val);
    void addInvite(int fd);
    void removeInvite(int fd);
    bool isInvited(int fd) const;

    bool hasKey() const;
    const std::string& getKey() const;
    void setKey(const std::string& k);

    bool hasLimit() const;
    size_t getLimit() const;
    void setLimit(size_t l);

    const std::vector<int>& getMembers() const;
    const std::vector<int>& getOperators() const;
};

#endif
