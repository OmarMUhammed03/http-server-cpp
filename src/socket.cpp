#include "socket.hpp"
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>

Socket::Socket(int fd) noexcept : fd_(fd) {}

Socket::~Socket() {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        reset(other.fd_);
        other.fd_ = -1;
    }
    return *this;
}

int Socket::fd() const noexcept {
    return fd_;
}

void Socket::reset(int new_fd) noexcept {
    if (fd_ >= 0) {
        ::close(fd_);
    }
    fd_ = new_fd;
}

int Socket::release() noexcept {
    int old = fd_;
    fd_ = constants::INVALID_FD;
    return old;
}

ssize_t Socket::send(const void* buf, size_t len, int flags) const {
    if (fd_ < 0) {
        return -1;
    }
    return ::send(fd_, buf, len, flags);
}

ssize_t Socket::recv(void* buf, size_t len, int flags) const {
    if (fd_ < 0) return -1;
    return ::recv(fd_, buf, len, flags);
}

bool Socket::set_nonblocking() const noexcept {
    if (fd_ < 0) return false;
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags < 0) return false;
    if (::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0) return false;
    return true;
}

void Socket::shutdown_write() const noexcept {
    if (fd_ >= 0) ::shutdown(fd_, SHUT_WR);
}