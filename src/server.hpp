#ifndef SERVER_HPP
#define SERVER_HPP

#include "constants.hpp"
#include "socket.hpp"
#include <cstdint>

class TcpServer {
public:
    TcpServer() = default;

    bool bind(uint16_t port);
    bool listen(int backlog);
    Socket accept() const;
    void handle_client(Socket& client) const;

private:
    Socket server_socket_{constants::INVALID_FD};
};

#endif
