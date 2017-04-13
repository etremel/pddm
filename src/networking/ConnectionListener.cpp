/**
 * @file ConnectionListener.cpp
 *
 * @date Oct 12, 2016
 * @author edward
 */

#include "ConnectionListener.h"

#include <cstring>
#include <memory>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Socket.h"

namespace pddm {
namespace networking {

ConnectionListener::ConnectionListener(int port) {
    sockaddr_in serv_addr;

    int listenfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) throw connection_failure("Couldn't create socket!");

    int reuse_addr = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_addr,
               sizeof(reuse_addr));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if(bind(listenfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        fprintf(stderr,
                "ERROR on binding to socket in ConnectionListener: %s\n",
                strerror(errno));
    listen(listenfd, 5);

    fd = std::unique_ptr<int, std::function<void(int *)>>(
        new int(listenfd), [](int *fd) { close(*fd); });
}

Socket ConnectionListener::accept() {
    char client_ip_cstr[INET6_ADDRSTRLEN + 1];
    struct sockaddr_storage client_addr_info;
    socklen_t len = sizeof client_addr_info;

    int sock = ::accept(*fd, (struct sockaddr *)&client_addr_info, &len);
    if(sock < 0) throw connection_failure("accept() failed!");

    if(client_addr_info.ss_family == AF_INET) {
        // Client has an IPv4 address
        struct sockaddr_in *s = (struct sockaddr_in *)&client_addr_info;
        inet_ntop(AF_INET, &s->sin_addr, client_ip_cstr, sizeof client_ip_cstr);
    } else {  // AF_INET6
        // Client has an IPv6 address
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_addr_info;
        inet_ntop(AF_INET6, &s->sin6_addr, client_ip_cstr,
                  sizeof client_ip_cstr);
    }

    return Socket(sock, std::string(client_ip_cstr));
}

} /* namespace networking */
} /* namespace pddm */
