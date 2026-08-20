#include "response.hpp"
#include "constants.hpp"

HttpResponse::HttpResponse(std::string_view raw) : raw_(raw) {}

HttpResponse HttpResponse::ok() {
    return HttpResponse{constants::HTTP_200_RESPONSE};
}

HttpResponse HttpResponse::not_found() {
    return HttpResponse{constants::HTTP_404_RESPONSE};
}

ssize_t HttpResponse::send(const Socket& client) const {
    return client.send(raw_.data(), raw_.size());
}
