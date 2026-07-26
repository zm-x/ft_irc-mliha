# ft_irc

ft_irc is a network programming project from the 42 curriculum that consists of building an Internet Relay Chat (IRC) server in C++98. The goal of the project is to understand how real-world network protocols work by implementing a fully functional IRC server compatible with standard IRC clients.

The server handles multiple simultaneous client connections using non-blocking sockets and I/O multiplexing (`poll()`), allowing users to communicate in real time through channels and private messages.

This project provides a hands-on introduction to:

- TCP/IP socket programming.
- Client-server architecture.
- Non-blocking I/O and event-driven programming.
- The IRC protocol (RFC 1459 / RFC 2812).
- Object-Oriented Programming in C++98.
- Multi-client connection management.
- Parsing and processing IRC commands.

## Features

- Multiple client connections.
- User authentication (`PASS`, `NICK`, `USER`).
- Channel management (`JOIN`, `PART`, `TOPIC`, `MODE`, `KICK`, `INVITE`).
- Private and channel messaging (`PRIVMSG`).
- Channel operator privileges.
- Password-protected channels.
- Invite-only channels.
- User limit management.
- Topic restrictions.
- Non-blocking socket communication using `poll()`.
- Compatibility with official IRC clients such as HexChat and LimeChat.

## Usage

```bash
make

./ircserv <port> <password>
```
Example:
```bash
./ircserv 6667 secret_password
```
Then connect using an IRC client:
```bash
/quote PASS secret_password
/NICK ur_name
/USER ur_name 0 * :ur_name
Project Objectives
```
The purpose of ft_irc is not only to build a chat server, but also to gain a deeper understanding of networking concepts and how internet protocols are designed and implemented. By recreating a simplified IRC server, this project demonstrates how modern communication systems manage users, channels, commands, and concurrent network connections.
