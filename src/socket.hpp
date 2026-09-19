#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "constants.hpp"
#include <sys/socket.h>

class Socket {
public:
    explicit Socket(int fd = constants::INVALID_FD) noexcept;
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    int fd() const noexcept;
    void reset(int new_fd = -1) noexcept;
    int release() noexcept;

    ssize_t send(const void* buf, size_t len, int flags = 0) const;

    ssize_t recv(void* buf, size_t len, int flags = 0) const;

    bool set_nonblocking() const noexcept;
    void shutdown_write() const noexcept;

private:
    int fd_;
};

#endif
