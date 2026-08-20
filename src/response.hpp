#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string_view>
#include "socket.hpp"

class HttpResponse {
public:
    static HttpResponse ok();

    static HttpResponse not_found();

    ssize_t send(const Socket& client) const;

private:
    explicit HttpResponse(std::string_view raw);

    std::string_view raw_;
};

#endif
