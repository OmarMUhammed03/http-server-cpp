#include "router.hpp"

void Router::add_route(std::string_view method, std::string_view path, Handler handler) {
    routes_.push_back({std::string(method), std::string(path), handler});
}

bool Router::match_route(const Route& route, std::string_view method, std::string_view path, std::string& out_body) const {
    if (route.method != method) return false;

    if (!route.path.empty() && route.path.back() == '*') {
        std::string prefix = route.path.substr(0, route.path.size() - 1);
        if (path.size() >= prefix.size() && path.substr(0, prefix.size()) == prefix) {
            out_body = path.substr(prefix.size());
            return true;
        }
    } else if (route.path == path) {
        return true;
    }

    return false;
}

HttpResponse Router::route(std::string_view method, std::string_view path, const std::string& body) const {
    for (const auto& route : routes_) {
        std::string matched_body;
        if (match_route(route, method, path, matched_body)) {
            return route.handler(matched_body);
        }
    }

    return HttpResponse::not_found();
}
