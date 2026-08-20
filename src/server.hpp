#ifndef SERVER_HPP
#define SERVER_HPP

#include "constants.hpp"
#include "socket.hpp"
#include "router.hpp"
#include <cstdint>
#include <string>

class TcpServer {
public:
    TcpServer() = default;

    bool bind(uint16_t port);
    bool listen(int backlog);
    Socket accept() const;
    void handle_client(Socket& client) const;

    void add_route(std::string_view method, std::string_view path, Router::Handler handler);

private:
    Socket server_socket_{constants::INVALID_FD};
    Router router_;
};

#endif
