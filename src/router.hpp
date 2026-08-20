#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <string>
#include <string_view>
#include <functional>
#include <vector>
#include "response.hpp"

class Router {
public:
    using Handler = std::function<HttpResponse(const std::string&)>;

    Router() = default;

    void add_route(std::string_view method, std::string_view path, Handler handler);
    HttpResponse route(std::string_view method, std::string_view path, const std::string& body) const;

private:
    struct Route {
        std::string method;
        std::string path;
        Handler handler;
    };

    std::vector<Route> routes_;

    bool match_route(const Route& route, std::string_view method, std::string_view path, std::string& out_body) const;
};

#endif
