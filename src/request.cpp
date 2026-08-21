#include "request.hpp"

std::string HttpRequest::get_header_value(const std::string &key) const {
    auto it = headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
}
