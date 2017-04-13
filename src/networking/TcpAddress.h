/**
 * @file TcpAddress.h
 *
 * @date Nov 7, 2016
 * @author edward
 */

#pragma once

#include <string>
#include <stdexcept>

namespace pddm {
namespace networking {


/**
 * Simple struct for binding together an IP address and port, since the standard
 * library can't parse a string in "a.b.c.d:port" format.
 */
struct TcpAddress {
        std::string ip_addr;
        int port;
};

bool operator<(const TcpAddress& lhs, const TcpAddress& rhs);

bool operator==(const TcpAddress& lhs, const TcpAddress& rhs);

/** Exception type for errors parsing user-input IP addresses */
class TcpParseException : public std::runtime_error {
    public:
        explicit TcpParseException(const std::string& what_arg) : std::runtime_error(what_arg) {}
        explicit TcpParseException(const char* what_arg) : std::runtime_error(what_arg) {}
};

TcpAddress parse_tcp_string(const std::string& ip_string);

}
}


