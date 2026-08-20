#include "response.hpp"
#include "constants.hpp"

HttpResponse::HttpResponse(std::string raw) : raw_(std::move(raw)) {}

HttpResponse HttpResponse::ok(std::string body) {
    std::string response = std::string(constants::HTTP_200_RESPONSE);
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "\r\n";
    response += body;
    return HttpResponse{std::move(response)};
}

HttpResponse HttpResponse::not_found() {
    std::string response = std::string(constants::HTTP_404_RESPONSE);
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: 0\r\n";
    response += "\r\n";
    return HttpResponse{std::move(response)};
}

ssize_t HttpResponse::send(const Socket& client) const {
    const char* data = raw_.data();
    size_t total_sent = 0;
    while (total_sent < raw_.size()) {
        ssize_t sent = client.send(data + total_sent, raw_.size() - total_sent);
        if (sent <= 0) {
            return sent;
        }
        total_sent += static_cast<size_t>(sent);
    }
    return static_cast<ssize_t>(total_sent);
}
