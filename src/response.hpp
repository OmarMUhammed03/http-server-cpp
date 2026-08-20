#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include "socket.hpp"

class HttpResponse {
public:
    static HttpResponse ok(std::string body);
    static HttpResponse not_found();

    ssize_t send(const Socket& client) const;

private:
    explicit HttpResponse(std::string raw);

    std::string raw_;
};

#endif
