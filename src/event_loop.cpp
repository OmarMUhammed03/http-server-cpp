#include "event_loop.hpp"
#include "constants.hpp"
#include <unistd.h>
#include <iostream>

EventLoop::EventLoop() : epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)) {
    if (epoll_fd_ < 0) {
        std::cerr << "epoll_create1 failed\n";
    }
}

EventLoop::~EventLoop() {
    if (epoll_fd_ >= 0) ::close(epoll_fd_);
}

bool EventLoop::add_fd(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "epoll_ctl ADD failed for fd " << fd << "\n";
        return false;
    }
    return true;
}

bool EventLoop::mod_fd(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
        std::cerr << "epoll_ctl MOD failed for fd " << fd << "\n";
        return false;
    }
    return true;
}

bool EventLoop::remove_fd(int fd) {
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        // not fatal - fd may already be closed
        return false;
    }
    return true;
}

int EventLoop::wait(epoll_event* events, int max_events, int timeout_ms) const {
    return ::epoll_wait(epoll_fd_, events, max_events, timeout_ms);
}
