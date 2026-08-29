 # ft_irc — Minimal IRC Server (C++98)

 A compact, standards-inspired Internet Relay Chat (IRC) server implemented in portable C++98. This project is a learning implementation used to explore TCP/IP socket programming, non-blocking I/O with `poll()`, and the core semantics of the IRC protocol (join/part, nick/user registration, channels, messaging, modes).

 This README gives a quick start, command reference, development notes and pointers for testing.

 Key highlights
 - Small, focused IRC server written for clarity and learning.
 - Non-blocking sockets with `poll()` to support many clients concurrently.
 - Channel support: `JOIN`, `PART`, `NAMES`, `TOPIC`, `MODE` (basic), `KICK`, `INVITE`.
 - User registration: `PASS`, `NICK`, `USER` and basic welcome numerics.
 - Private messages with `PRIVMSG` and simple operator controls.

 Requirements
 - Linux or macOS with a working C++ toolchain (g++/clang++)
 - Standard POSIX APIs (sockets, fcntl, poll)

 Build
 1. From the repository root run:

 ```bash
 make
 ```

 2. This produces the `ircserv` executable.

 Run

 ```bash
 ./ircserv <port> <password>
 ```

 Example:

 ```bash
 ./ircserv 6667 secret_password
 ```

 Connect with an IRC client (HexChat, WeeChat, irssi, etc.) or using `netcat` for quick checks.

 Quick connection (HexChat / CLI example):

 In the client send:
 ```
 PASS secret_password
 NICK alice
 USER alice 0 * :Alice Example
 ```

 Core supported commands
 - PASS <password>
 - NICK <nickname>
 - USER <username> <hostname> <servername> :<realname>
 - JOIN <#channel> [key]
 - PART <#channel> [:reason]
 - TOPIC <#channel> [ :<topic> ]
 - MODE <#channel> [+/-modes] [args]
 - KICK <#channel> <nick> [:reason]
 - INVITE <nick> <#channel>
 - PRIVMSG <target> :<message>
 - QUIT [:reason]

 Notes on behavior and protocol
 - Channels are created on `JOIN` and removed when empty.
 - `TOPIC` will return `ERR_NOSUCHCHANNEL` if the channel does not exist (no auto-create).
 - Topic changes may be restricted to operators (mode `+t`).
 - Mode handling is basic and supports `i` (invite-only), `t` (topic restrict), `k` (key/password), `l` (limit), `o` (operator).

 Project layout
 - `includes/` — headers (`Server.hpp`, `Channel.hpp`, `Client.hpp`).
 - `src/` — implementation files (`main.cpp`, `Server.cpp`, `Commands.cpp`, `Client.cpp`, `Channel.cpp`).
 - `Makefile` — build rules producing `ircserv`.

 Development notes
 - The code targets C++98 for pedagogy and portability. Modernizing to C++11+ is straightforward if desired.
 - Error replies follow IRC numeric conventions where implemented (e.g., `401`, `403`, `431`, `462`, `464`, `471`, `473`, `475`).
 - The server uses file descriptors as client identifiers internally; many helper methods use fd values to manage membership and operator status.

 Testing tips
 - Use two or more IRC clients to verify channel flows (join, message, topic, kick).
 - For quick protocol checks, `netcat` can be used:

 ```bash
 nc localhost 6667
 PASS secret_password
 NICK test
 USER test 0 * :Test User
 JOIN #room
 PRIVMSG #room :hello
 ```

 Contributing
 - Fixes and cleanups are welcome. Keep changes small and focused.
 - Prefer adding unit tests or simple integration checks when updating protocol behavior.

 License
 - MIT (or replace with your preferred license)

 Questions or improvements
 - Open an issue or send a patch with a concise description of the change and a short test case showing the behavior.
