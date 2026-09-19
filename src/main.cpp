#include <iostream>
#include <csignal>
#include <atomic>
#include "constants.hpp"
#include "server.hpp"
#include "response.hpp"

// Global pointer for signal handler to trigger graceful shutdown
static TcpServer* g_server = nullptr;

void signal_handler(int) {
    if (g_server) {
        std::cout << "\nShutting down...\n";
        g_server->shutdown();
    }
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "Logs from your program will appear here!\n";

    TcpServer server;
    g_server = &server;
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    // Ignore SIGPIPE - send() to closed socket shouldn't kill server
    std::signal(SIGPIPE, SIG_IGN);

    server.add_route(constants::HTTP_METHOD_GET, constants::HTTP_PATH_ROOT,
                     [](const HttpRequest &) { return HttpResponse::ok(""); });
    server.add_route(constants::HTTP_METHOD_GET, "/echo/*",
                     [](const HttpRequest &req) { return HttpResponse::ok(req.body); });
    server.add_route(constants::HTTP_METHOD_GET, "/user-agent",
                      [](const HttpRequest &req) { return HttpResponse::ok(req.get_header_value(constants::USER_AGENT)); });

    if (!server.bind(constants::PORT) || !server.listen(constants::BACKLOG)) {
        return 1;
    }

    std::cout << "Waiting for clients to connect...\n";
    server.run();

    return 0;
}
