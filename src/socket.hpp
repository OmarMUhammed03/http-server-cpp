#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/socket.h>

class Socket {
public:
    explicit Socket(int fd = -1) noexcept;
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    int fd() const noexcept;
    void reset(int new_fd = -1) noexcept;

    ssize_t send(const void* buf, size_t len, int flags = 0) const;

    ssize_t recv(void* buf, size_t len, int flags = 0) const;

private:
    int fd_;
};

#endif
