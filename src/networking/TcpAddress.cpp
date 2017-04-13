/**
 * @file TcpAddress.cpp
 *
 * @date Nov 7, 2016
 * @author edward
 */

#include "TcpAddress.h"

namespace pddm {
namespace networking {

bool operator<(const TcpAddress& lhs, const TcpAddress& rhs) {
    return lhs.ip_addr == rhs.ip_addr ?
            lhs.port < rhs.port :
            lhs.ip_addr < rhs.ip_addr;
}

bool operator==(const TcpAddress& lhs, const TcpAddress& rhs) {
    return lhs.ip_addr == rhs.ip_addr && lhs.port == rhs.port;
}

TcpAddress parse_tcp_string(const std::string& ip_string) {
    auto colon_pos = ip_string.find_first_of(':');
    if(colon_pos == std::string::npos)
        throw TcpParseException("No port delimiter (':') found in string.");
    std::string addr(ip_string.substr(0, colon_pos));
    int port;
    try {
        port = std::stoi(ip_string.substr(colon_pos+1));
    } catch (const std::invalid_argument& ex) {
        throw TcpParseException(std::string("Error parsing port number: ") + ex.what());
    } catch (const std::out_of_range& ex) {
        throw TcpParseException(std::string("Error parsing port number: ") + ex.what());
    }
    return TcpAddress{addr, port};
}


}
}

