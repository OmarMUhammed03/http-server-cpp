#include <iostream>
#include "server.hpp"
#include "response.hpp"

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "Logs from your program will appear here!\n";

    TcpServer server;
    if (!server.bind(4221) || !server.listen(5)) {
        return 1;
    }

    std::cout << "Waiting for a client to connect...\n";

    Socket client = server.accept();
    if (client.fd() < 0) {
        return 1;
    }

    std::cout << "Client connected\n";

    HttpResponse::ok().send(client);

    return 0;
}
