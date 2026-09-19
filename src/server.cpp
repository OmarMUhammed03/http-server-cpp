#include "server.hpp"
#include "response.hpp"
#include "socket.hpp"
#include "constants.hpp"
#include "request.hpp"
#include "thread_pool.hpp"
#include "event_loop.hpp"
#include <sstream>
#include <cstring>
#include <iostream>
#include <string_view>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <sys/epoll.h>

TcpServer::TcpServer() = default;

TcpServer::~TcpServer() {
    shutdown();
}

void TcpServer::add_route(std::string_view method, std::string_view path, Router::Handler handler) {
    router_.add_route(method, path, handler);
}

bool TcpServer::set_nonblocking(int fd) const {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return false;
    return true;
}

bool TcpServer::bind(uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return false;
    }

    int reuse = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        ::close(fd);
        return false;
    }

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (::bind(fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port " << port << "\n";
        ::close(fd);
        return false;
    }

    server_socket_.reset(fd);
    return true;
}

bool TcpServer::listen(int backlog) {
    if (server_socket_.fd() < 0) {
        return false;
    }
    if (::listen(server_socket_.fd(), backlog) != 0) {
        std::cerr << "listen failed\n";
        return false;
    }
    // Make listening socket non-blocking for epoll LT
    set_nonblocking(server_socket_.fd());
    return true;
}

Socket TcpServer::accept() const {
    if (server_socket_.fd() < 0) {
        return Socket{constants::INVALID_FD};
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd = ::accept4(server_socket_.fd(),
                            reinterpret_cast<struct sockaddr*>(&client_addr),
                            &client_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (client_fd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return Socket{constants::INVALID_FD};
        }
        std::cerr << "Failed to accept client: " << std::strerror(errno) << "\n";
        return Socket{constants::INVALID_FD};
    }

    return Socket{client_fd};
}

bool TcpServer::is_request_complete(const std::string& buf, size_t& headers_end, size_t& content_length) const {
    headers_end = buf.find(std::string(constants::HTTP_CRLF) + std::string(constants::HTTP_CRLF));
    if (headers_end == std::string::npos) return false;

    content_length = 0;
    // Quick scan for Content-Length
    size_t cl_pos = buf.find("Content-Length:");
    if (cl_pos != std::string::npos && cl_pos < headers_end) {
        size_t start = cl_pos + 15;
        while (start < buf.size() && (buf[start] == ' ' || buf[start] == '\t')) ++start;
        size_t end = buf.find("\r\n", start);
        if (end != std::string::npos && end <= headers_end) {
            try {
                content_length = std::stoul(buf.substr(start, end - start));
            } catch (...) {
                content_length = 0;
            }
        }
    }
    size_t body_start = headers_end + 4;
    size_t body_size = buf.size() > body_start ? buf.size() - body_start : 0;
    return body_size >= content_length;
}

HttpRequest TcpServer::parse_request(const std::string& request_text) const {
    HttpRequest request;
    size_t headers_end = request_text.find(std::string(constants::HTTP_CRLF) + std::string(constants::HTTP_CRLF));
    std::string headers = headers_end != std::string::npos
                          ? request_text.substr(0, headers_end)
                          : request_text;

    std::string first_line = headers.substr(0, headers.find(constants::HTTP_CRLF));
    std::stringstream first_line_stream(first_line);
    first_line_stream >> request.method >> request.path;

    size_t header_pos = headers.find(constants::HTTP_CRLF);
    while (header_pos != std::string::npos) {
        size_t next_pos = headers.find(constants::HTTP_CRLF, header_pos + 2);
        std::string header_line = headers.substr(header_pos + 2,
                                                   next_pos != std::string::npos ? next_pos - header_pos - 2 : std::string::npos);
        if (header_line.empty()) break;

        size_t colon = header_line.find(':');
        if (colon != std::string::npos) {
            std::string key = header_line.substr(0, colon);
            std::string value = header_line.substr(colon + 1);
            if (!value.empty() && value.front() == ' ') value.erase(0, 1);
            // Trim trailing \r if any
            if (!value.empty() && value.back() == '\r') value.pop_back();
            request.headers[key] = value;
        }

        header_pos = next_pos;
    }

    if (headers_end != std::string::npos && headers_end + 4 < request_text.size()) {
        size_t body_start = headers_end + 4;
        request.body = request_text.substr(body_start);
    }

    if (request.headers.count("Content-Length")) {
        size_t content_length = 0;
        try { content_length = std::stoul(request.headers["Content-Length"]); } catch (...) {}
        if (request.body.size() > content_length) {
            request.body = request.body.substr(0, content_length);
        }
    }

    return request;
}

void TcpServer::handle_queued_request(int client_fd, std::string raw_request) {
    Socket client(client_fd);
    if (client.fd() < 0) return;

    HttpRequest request = parse_request(raw_request);
    {
        std::lock_guard<std::mutex> lk(log_mutex_);
        std::cout << "Worker handling fd " << client_fd << " " << request.method << " " << request.path << std::endl;
    }

    HttpResponse response = router_.route(request);
    response.send(client);
    // Connection: close -> socket closed on Socket destructor
}

void TcpServer::close_client(int fd) {
    if (loop_) loop_->remove_fd(fd);
    conn_buffers_.erase(fd);
    ::close(fd);
}

void TcpServer::handle_client(Socket& client) const {
    // Legacy blocking handler - receives one request, routes, sends.
    // Used by unit tests and as worker fallback.
    char buffer[constants::BUFFER_SIZE];
    ssize_t bytes_received = client.recv(buffer, sizeof(buffer) - 1);
    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
        std::cerr << "Error receiving data: " << std::strerror(errno) << std::endl;
        return;
    }
    if (bytes_received == 0) return;
    buffer[bytes_received] = '\0';
    {
        // thread-safe log
        // const_cast to lock mutex in const method
        std::lock_guard<std::mutex> lk(const_cast<std::mutex&>(log_mutex_));
        std::cout << "Received data: " << buffer << std::endl;
    }

    std::string request_text(buffer, static_cast<size_t>(bytes_received));
    // If Content-Length indicates more body, try to read remaining with blocking recv (small grace for legacy path)
    // For event-driven path the reactor already buffered complete request.

    HttpRequest request = parse_request(request_text);
    HttpResponse response = router_.route(request);
    response.send(client);
}

void TcpServer::handle_client(Socket client) {
    Socket& ref = client;
    static_cast<const TcpServer*>(this)->handle_client(ref);
}

void TcpServer::run() {
    if (server_socket_.fd() < 0) {
        std::cerr << "Server socket not bound\n";
        return;
    }

    pool_ = std::make_unique<ThreadPool>(constants::THREAD_POOL_SIZE);
    loop_ = std::make_unique<EventLoop>();
    if (!loop_->valid()) {
        std::cerr << "EventLoop creation failed\n";
        return;
    }

    if (!loop_->add_fd(server_socket_.fd(), EPOLLIN)) {
        std::cerr << "Failed to add server socket to epoll\n";
        return;
    }

    {
        std::lock_guard<std::mutex> lk(log_mutex_);
        std::cout << "Event-driven server running on fd " << server_socket_.fd()
                  << " with " << constants::THREAD_POOL_SIZE << " workers, MAX_QUEUE="
                  << constants::MAX_QUEUE_SIZE << "\n";
    }

    epoll_event events[constants::MAX_EVENTS];

    while (running_.load()) {
        int nfds = loop_->wait(events, constants::MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            std::cerr << "epoll_wait error: " << std::strerror(errno) << "\n";
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == server_socket_.fd()) {
                // Accept all pending connections (LT, loop until EAGAIN)
                while (running_.load()) {
                    Socket client = accept();
                    if (client.fd() == constants::INVALID_FD) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        break;
                    }
                    int raw = client.release(); // steal fd without close
                    conn_buffers_[raw] = "";
                    if (!loop_->add_fd(raw, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                        std::cerr << "Failed to add client fd " << raw << " to epoll\n";
                        ::close(raw);
                        conn_buffers_.erase(raw);
                    } else {
                        std::lock_guard<std::mutex> lk(log_mutex_);
                        std::cout << "Accepted client fd " << raw << "\n";
                    }
                }
            } else {
                if (ev & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
                    close_client(fd);
                    continue;
                }
                if (ev & EPOLLIN) {
                    // Drain all available data into per-connection buffer
                    bool closed = false;
                    char buf[4096];
                    while (true) {
                        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
                        if (n > 0) {
                            conn_buffers_[fd].append(buf, static_cast<size_t>(n));
                        } else if (n == 0) {
                            closed = true;
                            break;
                        } else {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                            if (errno == EINTR) continue;
                            closed = true;
                            break;
                        }
                    }
                    if (closed) {
                        close_client(fd);
                        continue;
                    }

                    auto it = conn_buffers_.find(fd);
                    if (it == conn_buffers_.end()) continue;

                    size_t headers_end = 0, content_len = 0;
                    if (!is_request_complete(it->second, headers_end, content_len)) {
                        // Need more data - keep buffer, wait for next EPOLLIN
                        // Guard against header too large (DOS)
                        if (it->second.size() > 64 * 1024) {
                            std::cerr << "Header too large, closing fd " << fd << "\n";
                            close_client(fd);
                        }
                        continue;
                    }

                    // Complete request - extract exactly one request (Content-Length framing)
                    size_t body_start = headers_end + 4;
                    size_t total_needed = body_start + content_len;
                    std::string raw_request = it->second.substr(0, total_needed);
                    // Remainder (pipelined) stays in buffer - but we use Connection: close, so discard remainder
                    // For keep-alive future: keep remainder
                    // Remove from epoll now so no more EPOLLIN before worker closes
                    loop_->remove_fd(fd);
                    conn_buffers_.erase(it);

                    // Enqueue to thread pool with bounded check
                    auto task = [this, fd, req = std::move(raw_request)]() mutable {
                        handle_queued_request(fd, std::move(req));
                    };

                    if (!pool_->try_enqueue(std::move(task))) {
                        rejected_.fetch_add(1);
                        size_t q = pool_->queue_size();
                        {
                            std::lock_guard<std::mutex> lk(log_mutex_);
                            std::cerr << "REJECT queue full 10k (queue=" << q << ") fd " << fd
                                      << " rejected=" << rejected_.load() << "\n";
                        }
                        {
                            Socket tmp(fd);
                            HttpResponse::service_unavailable().send(tmp);
                            // tmp closes fd on destructor - do not double close
                        }
                        // already removed from epoll
                    } else {
                        size_t q = pool_->queue_size();
                        if (q > 8000) {
                            std::lock_guard<std::mutex> lk(log_mutex_);
                            std::cout << "WARN queue high: " << q << " / " << constants::MAX_QUEUE_SIZE << "\n";
                        }
                    }
                }
            }
        }
    }

    // Cleanup
    for (auto& kv : conn_buffers_) {
        ::close(kv.first);
    }
    conn_buffers_.clear();
}

void TcpServer::shutdown() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) return;
    // Wake epoll_wait by closing server socket or sending dummy event
    // Close server socket to unblock accept, and also close epoll fd via loop reset
    if (server_socket_.fd() >= 0) {
        ::shutdown(server_socket_.fd(), SHUT_RDWR);
    }
    if (pool_) pool_->shutdown();
    // EventLoop will be destroyed, closing epoll fd and waking wait with EBADF
}
