*This project has been created as part of the 42 curriculum by zael-mou, modryouc.*

# ft_irc

A custom Internet Relay Chat (IRC) server built in C++98, fully compliant with core IRC specifications (RFC 1459 and RFC 2812) and designed to support standard IRC clients such as Irssi and HexChat.

---

## Description

The goal of ft_irc is to implement an IRC server from scratch using non-blocking I/O multiplexing (`poll()` / `epoll()` / `select()`) in C++98. The server handles multiple concurrent TCP connections, authentication workflows, channel operations, user permission levels, and private or group messaging.

### Key Features
- **Non-Blocking Architecture:** Uses a single `poll()` event loop to handle simultaneous connections, reads, and writes without blocking the server.
- **Authentication System:** Strict registration lifecycle verifying `PASS`, `NICK`, and `USER` commands before granting access.
- **Channel Operations:** Full support for `JOIN`, `PART`, `TOPIC`, `INVITE`, and `KICK`.
- **Channel Modes:**
  - `+i` / `-i`: Invite-only channel toggle.
  - `+t` / `-t`: Topic restrictions (operators only vs. all members).
  - `+k` / `-k`: Channel key (password) protection.
  - `+o` / `-o`: Grant/revoke channel operator privileges.
  - `+l` / `-l`: User capacity limit enforcement.
- **Real-Time Messaging:** Direct messaging and channel broadcasting via `PRIVMSG`.
- **RFC Error Codes:** Comprehensive implementation of numeric replies and error codes (`401`, `403`, `431`, `432`, `433`, `451`, `461`, `471`, `473`, `475`, `482`, etc.).

---

## Instructions

### Prerequisites
- C++ compiler (`c++` or `g++` or `clang++`) supporting C++98 standards.
- `make` build utility.
- An IRC client for testing (e.g., `irssi`, `HexChat`, or `nc` / `netcat`).

### Compilation
Clone the repository and build the binary:
```bash
make