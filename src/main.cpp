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
                     [](const HttpRequest &) { return HttpResponse::ok(""); });
    server.add_route(constants::HTTP_METHOD_GET, "/echo/*",
                     [](const HttpRequest &req) { return HttpResponse::ok(req.body); });
    server.add_route(constants::HTTP_METHOD_GET, "/user-agent",
                      [](const HttpRequest &req) { return HttpResponse::ok(req.get_header_value(constants::USER_AGENT)); });

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
