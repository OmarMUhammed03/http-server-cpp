#include "response.hpp"

HttpResponse::HttpResponse(std::string_view raw) : raw_(raw) {}

HttpResponse HttpResponse::ok() {
    return HttpResponse{"HTTP/1.1 200 OK\r\n\r\n"};
}

ssize_t HttpResponse::send(const Socket& client) const {
    return client.send(raw_.data(), raw_.size());
}
