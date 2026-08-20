#include <iostream>
#include "constants.hpp"
#include "server.hpp"
#include "response.hpp"

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "Logs from your program will appear here!\n";

    TcpServer server;
    server.add_route(constants::HTTP_METHOD_GET, constants::HTTP_PATH_ROOT,
                     [](const std::string&) { return HttpResponse::ok(""); });
    server.add_route(constants::HTTP_METHOD_GET, "/echo/*",
                     [](const std::string& body) { return HttpResponse::ok(body); });

    if (!server.bind(constants::PORT) || !server.listen(constants::BACKLOG)) {
        return 1;
    }

    std::cout << "Waiting for a client to connect...\n";

    Socket client = server.accept();
    if (client.fd() == constants::INVALID_FD) {
        return 1;
    }

    std::cout << "Client connected\n";
    server.handle_client(client);

    return 0;
}
