#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

namespace constants {
    constexpr uint16_t PORT = 4221;
    constexpr int BACKLOG = 5;
    constexpr int INVALID_FD = -1;
    constexpr std::size_t BUFFER_SIZE = 4096;
    inline const std::string USER_AGENT = "User-Agent";
    inline const std::string_view HTTP_CRLF = "\r\n";
    inline constexpr std::string_view HTTP_200_RESPONSE = "HTTP/1.1 200 OK\r\n";
    inline constexpr std::string_view HTTP_404_RESPONSE = "HTTP/1.1 404 Not Found\r\n";
    inline constexpr std::string_view HTTP_METHOD_GET = "GET";
    inline constexpr std::string_view HTTP_PATH_ROOT = "/";
    inline constexpr std::string_view HTTP_PATH_ECHO = "/echo/";

}

#endif
