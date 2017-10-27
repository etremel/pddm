/**
 * @file Socket.cpp
 *
 * @date Oct 12, 2016
 * @author edward
 */

#include "Socket.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>

namespace pddm {
namespace networking {

Socket::Socket(std::string servername, int port) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) throw connection_failure("Failed to create socket!");

    hostent* server;
    server = gethostbyname(servername.c_str());
    if(server == nullptr) throw connection_failure(std::string("Error: Could not find host ") + servername);

    char server_ip_cstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, server->h_addr, server_ip_cstr, sizeof(server_ip_cstr));
    remote_ip = std::string(server_ip_cstr);

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);

    while(connect(sock, (sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        /* do nothing*/;
    
}

Socket::Socket(Socket &&s) : sock(s.sock), remote_ip(s.remote_ip) {
    s.sock = -1;
    s.remote_ip = std::string();
}

Socket& Socket::operator=(Socket &&s) {
    sock = s.sock;
    s.sock = -1;
    remote_ip = std::move(s.remote_ip);
    return *this;
}

Socket::~Socket() {
    if(sock >= 0) close(sock);
}

bool Socket::is_empty() { return sock == -1; }

bool Socket::read(char *buffer, size_t size) {
    if(sock < 0) {
        fprintf(stderr, "WARNING: Attempted to read from closed socket\n");
        return false;
    }

    size_t total_bytes = 0;
    while(total_bytes < size) {
        ssize_t new_bytes = ::read(sock, buffer + total_bytes, size - total_bytes);
        if(new_bytes > 0) {
            total_bytes += new_bytes;
        } else if(new_bytes == 0 || (new_bytes == -1 && errno != EINTR)) {
            return false;
        }
    }
    return true;
}

bool Socket::probe() {
    int count;
    ioctl(sock, FIONREAD, &count);
    return count > 0;
}

bool Socket::write(const char *buffer, size_t size) {
    if(sock < 0) {
        fprintf(stderr, "WARNING: Attempted to write to closed socket\n");
        return false;
    }

    size_t total_bytes = 0;
    while(total_bytes < size) {
        ssize_t bytes_written = ::write(sock, buffer + total_bytes, size - total_bytes);
        if(bytes_written >= 0) {
            total_bytes += bytes_written;
        } else if(bytes_written == -1 && errno != EINTR) {
            return false;
        }
    }
    return true;
}

} /* namespace networking */
} /* namespace pddm */
