#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
private:
    int         _fd;
    std::string _inBuffer;
    std::string _outBuffer;

    std::string _nickname;
    std::string _username;
    std::string _realname;

    bool        _authenticated;
    bool        _registered;

public:
    Client();
    Client(int fd);
    ~Client();

    int  getFd() const;
    void setFd(int fd);

    void appendToInBuffer(const std::string& data);
    bool popOneLine(std::string& line);

    void queueMessage(const std::string& msg);
    std::string& outBuffer();

    const std::string& getNickname() const;
    const std::string& getUsername() const;
    const std::string& getRealname() const;

    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username);
    void setRealname(const std::string& realname);

    bool isAuthenticated() const;
    bool isRegistered() const;

    void setAuthenticated(bool value);
    void setRegistered(bool value);
};

#endif