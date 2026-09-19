#ifndef EVENT_LOOP_HPP
#define EVENT_LOOP_HPP

#include <sys/epoll.h>
#include <unordered_map>
#include <string>

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    bool add_fd(int fd, uint32_t events);
    bool mod_fd(int fd, uint32_t events);
    bool remove_fd(int fd);
    int wait(epoll_event* events, int max_events, int timeout_ms) const;
    int fd() const noexcept { return epoll_fd_; }
    bool valid() const noexcept { return epoll_fd_ >= 0; }

private:
    int epoll_fd_;
};

#endif
