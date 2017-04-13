/**
 * @file Socket.h
 *
 * @date Oct 12, 2016
 * @author edward
 */

#pragma once

#include <string>
#include <cstddef>
#include <stdexcept>

namespace pddm {
namespace networking {

class connection_failure : public std::runtime_error {
    public:
        connection_failure(const std::string& what_arg) : std::runtime_error(what_arg) {}
};


/**
 * A simple C++ wrapper around the horrendously ugly C socket interface. Mostly
 * written by Jonathan Behrens, forked from the Derecho project.
 */
class Socket {
    private:
        int sock;

        explicit Socket(int _sock) : sock(_sock), remote_ip() {}
        explicit Socket(int _sock, std::string remote_ip) :
                sock(_sock), remote_ip(remote_ip) {}

    public:
        std::string remote_ip;

        Socket() : sock(-1), remote_ip() {}
        /** Constructs a socket connected to the given hostname and port. */
        Socket(std::string servername, int port);
        Socket(Socket&& s);

        Socket& operator=(Socket& s) = delete;
        Socket& operator=(Socket&& s);

        ~Socket();

        bool is_empty();

        /** Reads size bytes from the socket into buffer */
        bool read(char* buffer, size_t size);
        bool probe();
        /** Writes size bytes from buffer into the socket. */
        bool write(char const* buffer, size_t size);

        friend class ConnectionListener;

};

} /* namespace networking */
} /* namespace pddm */

