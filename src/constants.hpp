#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

namespace constants {
    constexpr uint16_t PORT = 4221;
    constexpr int BACKLOG = 128;
    constexpr int INVALID_FD = -1;
    constexpr std::size_t BUFFER_SIZE = 4096;
    constexpr std::size_t THREAD_POOL_SIZE = 8;
    constexpr std::size_t MAX_QUEUE_SIZE = 10000;
    constexpr int MAX_EVENTS = 64;
    inline const std::string USER_AGENT = "User-Agent";
    inline const std::string_view HTTP_CRLF = "\r\n";
    inline constexpr std::string_view HTTP_200_RESPONSE = "HTTP/1.1 200 OK\r\n";
    inline constexpr std::string_view HTTP_404_RESPONSE = "HTTP/1.1 404 Not Found\r\n";
    inline constexpr std::string_view HTTP_503_RESPONSE = "HTTP/1.1 503 Service Unavailable\r\n";
    inline constexpr std::string_view HTTP_METHOD_GET = "GET";
    inline constexpr std::string_view HTTP_PATH_ROOT = "/";
    inline constexpr std::string_view HTTP_PATH_ECHO = "/echo/";

}

#endif
