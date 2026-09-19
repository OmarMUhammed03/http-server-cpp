#ifndef SERVER_HPP
#define SERVER_HPP

#include "constants.hpp"
#include "socket.hpp"
#include "router.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <memory>

class ThreadPool;
class EventLoop;

class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    bool bind(uint16_t port);
    bool listen(int backlog);
    Socket accept() const;
    void handle_client(Socket& client) const; // legacy single-recv (used by workers)
    void handle_client(Socket client);        // ownership version delegates to above
    void add_route(std::string_view method, std::string_view path, Router::Handler handler);

    // Event-driven entry point: epoll reactor + thread pool
    void run();
    void shutdown();

private:
    bool set_nonblocking(int fd) const;
    bool is_request_complete(const std::string& buf, size_t& headers_end, size_t& content_length) const;
    HttpRequest parse_request(const std::string& raw) const;
    void handle_queued_request(int client_fd, std::string raw_request);
    void close_client(int fd);

    Socket server_socket_{constants::INVALID_FD};
    Router router_;

    std::unique_ptr<ThreadPool> pool_;
    std::unique_ptr<EventLoop> loop_;
    std::atomic<bool> running_{true};
    std::unordered_map<int, std::string> conn_buffers_;
    mutable std::mutex log_mutex_;
    std::atomic<size_t> rejected_{0};
};

#endif
