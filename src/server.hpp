#ifndef SERVER_HPP
#define SERVER_HPP

#include "socket.hpp"
#include <cstdint>

class TcpServer {
public:
    TcpServer() = default;

    bool bind(uint16_t port);
    bool listen(int backlog);
    Socket accept() const;

private:
    Socket server_socket_{-1};
};

#endif
