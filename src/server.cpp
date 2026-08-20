#include "server.hpp"
#include "response.hpp"
#include "socket.hpp"
#include "constants.hpp"
#include <sstream>
#include <cstring>
#include <iostream>
#include <string_view>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

void TcpServer::add_route(std::string_view method, std::string_view path, Router::Handler handler) {
    router_.add_route(method, path, handler);
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
    return true;
}

Socket TcpServer::accept() const {
    if (server_socket_.fd() < 0) {
        return Socket{constants::INVALID_FD};
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd = ::accept(server_socket_.fd(),
                            reinterpret_cast<struct sockaddr*>(&client_addr),
                            &client_addr_len);
    if (client_fd < 0) {
        std::cerr << "Failed to accept client\n";
        return Socket{constants::INVALID_FD};
    }

    return Socket{client_fd};
}

void TcpServer::handle_client(Socket& client) const {
    char buffer[constants::BUFFER_SIZE];
    ssize_t bytes_received = client.recv(buffer, sizeof(buffer) - 1);
    if (bytes_received < 0) {
        std::cerr << "Error receiving data: " << std::strerror(errno) << std::endl;
        return;
    }
    buffer[bytes_received] = '\0';
    std::cout << "Received data: " << buffer << std::endl;
    std::string request_text(buffer);
    std::string first_line = request_text.substr(0, request_text.find(constants::HTTP_CRLF));
    std::stringstream response_stream(first_line);

    std::string method, path;
    response_stream >> method >> path;

    HttpResponse response = router_.route(method, path, "");
    response.send(client);
}
