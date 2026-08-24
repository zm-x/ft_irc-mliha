# ft_irc System Calls Documentation

## Overview
This document explains all system calls (syscalls) used in the ft_irc project. System calls are functions that provide an interface between user-space programs and the operating system kernel.

---

## 1. socket()

### Purpose
Creates a new socket (a communication endpoint) for network communication.

### Syntax
```c
#include <sys/socket.h>

int socket(int domain, int type, int protocol);
```

### Arguments
- **domain**: Address family (protocol family)
  - `AF_INET`: IPv4 internet protocols
  - `AF_INET6`: IPv6 internet protocols
  - `AF_UNIX`: Unix domain sockets
  
- **type**: Socket type
  - `SOCK_STREAM`: Provides TCP (reliable, ordered, connection-oriented)
  - `SOCK_DGRAM`: Provides UDP (unreliable, datagram-based)
  
- **protocol**: Specific protocol within the domain
  - `0`: Use default protocol for the domain/type combination

### Return Value
- On success: Returns a file descriptor (non-negative integer)
- On error: Returns `-1` and sets `errno`

### Example from ft_irc
```cpp
_listenFd = socket(AF_INET, SOCK_STREAM, 0);
if (_listenFd < 0)
    throw std::runtime_error("socket failed");
```

### Explanation
Creates a TCP socket for IPv4 communication. This socket will listen for incoming client connections on a specific port.

---

## 2. setsockopt()

### Purpose
Sets options on a socket (modify socket behavior).

### Syntax
```c
#include <sys/socket.h>

int setsockopt(int sockfd, int level, int optname,
               const void *optval, socklen_t optlen);
```

### Arguments
- **sockfd**: Socket file descriptor returned by `socket()`
- **level**: Level at which the option resides
  - `SOL_SOCKET`: Socket level options
  - `IPPROTO_TCP`: TCP protocol level
  - `IPPROTO_IP`: IP protocol level
  
- **optname**: Option name to set
  - `SO_REUSEADDR`: Allows reusing local address in TIME_WAIT state
  - `SO_REUSEPORT`: Allows multiple sockets to bind to same port
  - `SO_KEEPALIVE`: Enable TCP keep-alive
  
- **optval**: Pointer to the option value
- **optlen**: Length of the option value

### Return Value
- On success: Returns `0`
- On error: Returns `-1` and sets `errno`

### Example from ft_irc
```cpp
int yes = 1;
if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
    throw std::runtime_error("setsockopt failed");
```

### Explanation
Sets `SO_REUSEADDR` to allow the socket to bind to a port that is in TIME_WAIT state. This is useful during development/testing to restart the server without waiting.

---

## 3. fcntl()

### Purpose
Perform operations on file descriptors (e.g., set non-blocking mode).

### Syntax
```c
#include <fcntl.h>

int fcntl(int fd, int cmd, ... /* arg */);
```

### Arguments
- **fd**: File descriptor to operate on
- **cmd**: Command to execute
  - `F_GETFL`: Get file descriptor flags
  - `F_SETFL`: Set file descriptor flags
  - `F_GETFD`: Get file descriptor-specific flags
  - `F_SETFD`: Set file descriptor-specific flags
  
- **arg**: Varies depending on command
  - For `F_SETFL`: Flags to set (e.g., `O_NONBLOCK`)

### Return Value
- Depends on the command; usually returns `0` on success or `-1` on error

### Example from ft_irc
```cpp
if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
    throw std::runtime_error("fcntl failed");
```

### Explanation
Sets the socket to non-blocking mode. This means operations like `recv()` and `send()` will not block; they'll return immediately with `EAGAIN` or `EWOULDBLOCK` if no data is available.

### O_NONBLOCK Flag
- Makes the file descriptor non-blocking
- Operations that would normally wait will return immediately with an error instead

---

## 4. bind()

### Purpose
Assigns a local address (IP + port) to a socket.

### Syntax
```c
#include <sys/socket.h>

int bind(int sockfd, const struct sockaddr *addr,
         socklen_t addrlen);
```

### Arguments
- **sockfd**: Socket file descriptor
- **addr**: Pointer to `sockaddr` structure containing:
  - Address family
  - Port number
  - IP address
  
- **addrlen**: Size of the address structure

### Return Value
- On success: Returns `0`
- On error: Returns `-1` and sets `errno`

### Example from ft_irc
```cpp
struct sockaddr_in addr;
std::memset(&addr, 0, sizeof(addr));
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any interface
addr.sin_port = htons(_port);        // Convert port to network byte order

if (bind(_listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    throw std::runtime_error("bind failed");
```

### Explanation
Binds the listening socket to all available network interfaces (`INADDR_ANY`) on the specified port. `htons()` converts the port number to network byte order (big-endian).

---

## 5. listen()

### Purpose
Marks a socket as a passive socket, ready to accept incoming connections.

### Syntax
```c
#include <sys/socket.h>

int listen(int sockfd, int backlog);
```

### Arguments
- **sockfd**: Socket file descriptor (should be bound with `bind()`)
- **backlog**: Maximum number of pending connections in the queue
  - Kernel maintains queue of unaccepted connections
  - `SOMAXCONN`: Use system maximum (typically 128)
  - Higher values may be clamped by the system

### Return Value
- On success: Returns `0`
- On error: Returns `-1` and sets `errno`

### Example from ft_irc
```cpp
if (listen(_listenFd, SOMAXCONN) < 0)
    throw std::runtime_error("listen failed");
```

### Explanation
Puts the listening socket into passive mode. Now the kernel will queue incoming connection attempts, allowing the server to accept them with `accept()`.

---

## 6. accept()

### Purpose
Accepts an incoming connection on a listening socket and creates a new socket for communication with the client.

### Syntax
```c
#include <sys/socket.h>

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

### Arguments
- **sockfd**: Listening socket file descriptor
- **addr**: Pointer to structure that will receive client's address
  - Can be `NULL` if you don't need client info
  
- **addrlen**: Pointer to length of address structure
  - Must be initialized with the size of the `addr` structure
  - On return, contains the actual size of the client address

### Return Value
- On success: Returns new socket file descriptor for communication
- On error: Returns `-1` and sets `errno`
  - `EAGAIN`/`EWOULDBLOCK`: No pending connections (non-blocking mode)

### Example from ft_irc
```cpp
struct sockaddr_in cliAddr;
socklen_t len = sizeof(cliAddr);
int cfd = accept(_listenFd, (struct sockaddr*)&cliAddr, &len);
if (cfd < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) return;  // Non-blocking, no connection
    std::cerr << "accept error\n";
    return;
}
```

### Explanation
Accepts a pending connection from a client. Returns a new file descriptor for communicating with that specific client. The original listening socket continues to listen for more connections.

---

## 7. poll()

### Purpose
Monitors multiple file descriptors to see if they're ready for I/O operations.

### Syntax
```c
#include <poll.h>

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

### Arguments
- **fds**: Array of `pollfd` structures
- **nfds**: Number of elements in the `fds` array
- **timeout**: Maximum time to wait in milliseconds
  - `-1`: Wait indefinitely
  - `0`: Return immediately (non-blocking)
  - Positive number: Wait that many milliseconds

### pollfd Structure
```c
struct pollfd {
    int fd;          // File descriptor to monitor
    short events;    // Events to monitor (input mask)
    short revents;   // Events that occurred (output mask)
};
```

### Common Event Flags
- **Input events:**
  - `POLLIN`: Data available to read
  - `POLLHUP`: Connection closed by peer
  - `POLLERR`: Error condition
  - `POLLNVAL`: Invalid file descriptor

- **Output events:**
  - `POLLOUT`: Ready for writing

### Return Value
- On success: Number of file descriptors with non-zero `revents`
- On timeout: Returns `0`
- On error: Returns `-1` and sets `errno`

### Example from ft_irc
```cpp
int ret = poll(&_pollfds[0], _pollfds.size(), -1);
if (ret < 0) {
    if (errno == EINTR) continue;  // Interrupted by signal
    throw std::runtime_error("poll failed");
}

for (size_t i = 0; i < _pollfds.size(); ++i) {
    if (_pollfds[i].revents)
        handlePollEvent(i);  // Process ready file descriptors
}
```

### Explanation
Waits for events on multiple sockets. Once `poll()` returns, check each socket's `revents` to see what happened. This allows handling many clients efficiently.

---

## 8. recv()

### Purpose
Receives data from a connected socket.

### Syntax
```c
#include <sys/socket.h>

ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

### Arguments
- **sockfd**: Socket file descriptor to receive from
- **buf**: Buffer to store received data
- **len**: Maximum number of bytes to receive
- **flags**: Optional flags
  - `0`: Default behavior
  - `MSG_DONTWAIT`: Non-blocking operation
  - `MSG_PEEK`: Peek at data without removing it

### Return Value
- On success: Number of bytes received (0 means connection closed)
- On error: Returns `-1` and sets `errno`
  - `EAGAIN`/`EWOULDBLOCK`: No data available (non-blocking mode)

### Example from ft_irc
```cpp
char buffer[512];
ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
if (n <= 0) {
    if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
        disconnectClient(fd);  // Connection closed or error
    return;
}
Client& c = _clients[fd];
c.appendToInBuffer(std::string(buffer, n));
```

### Explanation
Reads up to 512 bytes of data from the client socket. If the connection is closed (n=0) or an error occurs (except non-blocking), disconnect the client.

---

## 9. send()

### Purpose
Sends data through a connected socket.

### Syntax
```c
#include <sys/socket.h>

ssize_t send(int sockfd, const void *buf, size_t len, int flags);
```

### Arguments
- **sockfd**: Socket file descriptor to send to
- **buf**: Pointer to data to send
- **len**: Number of bytes to send
- **flags**: Optional flags
  - `0`: Default behavior
  - `MSG_DONTWAIT`: Non-blocking send
  - `MSG_NOSIGNAL`: Don't send SIGPIPE on error

### Return Value
- On success: Number of bytes sent (may be less than requested)
- On error: Returns `-1` and sets `errno`
  - `EAGAIN`/`EWOULDBLOCK`: Send buffer full (non-blocking mode)

### Example from ft_irc
```cpp
std::string& out = c.outBuffer();
if (out.empty()) {
    updatePollOutEvent(fd, false);
    return;
}
ssize_t n = send(fd, out.c_str(), out.size(), 0);
if (n < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
        disconnectClient(fd);  // Error occurred
    return;
}
out.erase(0, n);  // Remove sent bytes from buffer
```

### Explanation
Sends queued data to the client. If only some bytes are sent, remove them from the buffer and queue will be sent on next `poll()` event.

---

## 10. close()

### Purpose
Closes a file descriptor and associated socket.

### Syntax
```c
#include <unistd.h>

int close(int fd);
```

### Arguments
- **fd**: File descriptor to close

### Return Value
- On success: Returns `0`
- On error: Returns `-1` and sets `errno`

### Example from ft_irc
```cpp
~Server() {
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        close(it->first);  // Close all client sockets
    if (_listenFd >= 0)
        close(_listenFd);  // Close listening socket
}

void Server::disconnectClient(int fd) {
    std::cout << "Client disconnected fd=" << fd << std::endl;
    close(fd);           // Close the socket
    _clients.erase(fd);  // Remove from clients map
    removePollFd(fd);    // Remove from poll array
}
```

### Explanation
Closes a socket when it's no longer needed. Frees the file descriptor so it can be reused. For TCP sockets, initiates the close sequence.

---

## System Call Flow in ft_irc

### Server Startup
1. **socket()** → Create listening socket
2. **setsockopt()** → Configure socket (SO_REUSEADDR)
3. **fcntl()** → Set non-blocking mode
4. **bind()** → Bind to port
5. **listen()** → Mark as listening
6. **poll()** → Add listening socket to poll array

### Client Connection
1. **poll()** → Detects incoming connection (POLLIN on listening socket)
2. **accept()** → Creates new client socket
3. **fcntl()** → Set new socket to non-blocking
4. **poll()** → Add client socket to poll array

### Client Communication
1. **poll()** → Waits for events on all sockets
2. **recv()** → Read data from client (POLLIN event)
3. **send()** → Write data to client (POLLOUT event)

### Client Disconnection
1. **poll()** → Detects closure (POLLHUP or error)
2. **close()** → Close the socket
3. **poll()** → Remove from poll array

---

## Key Concepts

### Non-Blocking I/O
- Sockets are set to non-blocking mode using `fcntl(fd, F_SETFL, O_NONBLOCK)`
- Operations return immediately, allowing the server to handle multiple clients
- Errors `EAGAIN`/`EWOULDBLOCK` indicate operation would have blocked

### Poll-Based Multiplexing
- Single thread monitors multiple client connections
- More efficient than creating thread per client
- `poll()` efficiently waits for any socket to become ready
- Scales well with many concurrent connections

### Error Handling
- Always check return values of system calls
- Distinguish between recoverable errors (`EAGAIN`, `EINTR`) and fatal errors
- Use `errno` for error details
- Proper cleanup in destructors to avoid resource leaks

---

## Error Constants

| Constant | Meaning |
|----------|---------|
| `EAGAIN` | Resource temporarily unavailable (non-blocking I/O) |
| `EWOULDBLOCK` | Same as EAGAIN on most systems |
| `EINTR` | Interrupted system call (resume) |
| `EADDRINUSE` | Address already in use |
| `ECONNREFUSED` | Connection refused |
| `EPIPE` | Broken pipe (send on closed socket) |
| `ETIMEDOUT` | Connection timed out |

---

## Summary Table

| Syscall | Purpose | Key Args | Returns |
|---------|---------|----------|---------|
| socket() | Create socket | domain, type, protocol | fd or -1 |
| setsockopt() | Configure socket | fd, level, optname, value | 0 or -1 |
| fcntl() | File descriptor operations | fd, cmd, flags | varies |
| bind() | Assign local address | fd, address structure | 0 or -1 |
| listen() | Mark as listening | fd, backlog | 0 or -1 |
| accept() | Accept connection | listen_fd, client_addr | new_fd or -1 |
| poll() | Monitor file descriptors | fds array, timeout | count or -1 |
| recv() | Receive data | fd, buffer, length | bytes or -1 |
| send() | Send data | fd, buffer, length | bytes or -1 |
| close() | Close file descriptor | fd | 0 or -1 |

