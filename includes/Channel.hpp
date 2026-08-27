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

public:
    Channel();
    Channel(const std::string& name);
    ~Channel();

    const std::string& getName() const;

    const std::string& getTopic() const;
    void setTopic(const std::string& topic);

    void addMember(int fd);
    void removeMember(int fd);
    bool hasMember(int fd) const;

    void addOperator(int fd);
    void removeOperator(int fd);
    bool isOperator(int fd) const;

    const std::vector<int>& getMembers() const;
    const std::vector<int>& getOperators() const;
};

#endif