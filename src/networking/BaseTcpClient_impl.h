/**
 * @file BaseTcpClient.cpp
 *
 * @date Apr 13, 2017
 * @author edward
 */

#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>

#include "BaseTcpClient.h"
#include "Socket.h"

namespace pddm {
namespace networking {

template<typename Impl>
BaseTcpClient<Impl>::BaseTcpClient(Impl* subclass_this, const TcpAddress& my_address,
        const std::map<int, TcpAddress>& meter_ips_by_id) :
        impl_this(subclass_this),
        id_to_ip_map(meter_ips_by_id) {
    //Create socket
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket_fd < 0) throw connection_failure("Could not create a socket to listen for incoming connections.");
    int reuse_addr = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_addr, sizeof(reuse_addr));

    //Bind socket to my port
    sockaddr_in my_address_c;
    memset(&my_address_c, 0, sizeof(my_address_c));
    my_address_c.sin_family = AF_INET;
    my_address_c.sin_addr.s_addr = INADDR_ANY;
    my_address_c.sin_port = htons(my_address.port);
    if(bind(server_socket_fd, (sockaddr*) &my_address_c, sizeof(my_address_c)) < 0) {
        fprintf(stderr, "Error binding to socket on port %d: %s", my_address.port, strerror(errno));
        throw connection_failure("Bind failure.");
    }
    //Make socket nonblocking
    int flags;
    flags = fcntl(server_socket_fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if(fcntl(server_socket_fd, F_SETFL, flags) < 0) {
        throw connection_failure("Failed to set listening socket to nonblocking!");
    }

    //Setup epoll to listen on socket
    listen(server_socket_fd, SOMAXCONN);
    epoll_fd = epoll_create1(0);
    if(epoll_fd < 0) throw connection_failure("Could not create an epoll instance in TcpNetworkClient.");
    struct epoll_event event;
    event.data.fd = server_socket_fd;
    event.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket_fd, &event);
}

template<typename Impl>
BaseTcpClient<Impl>::~BaseTcpClient() {
    close(server_socket_fd);
}

template<typename Impl>
void BaseTcpClient<Impl>::monitor_incoming_messages() {
    const int EVENTS_LENGTH = 64;
    struct epoll_event* events = (struct epoll_event*) calloc(EVENTS_LENGTH, sizeof(struct epoll_event));
    //Indexed by FD, since that's all the information we get when a socket has data
    std::map<int, std::vector<char>> sender_buffers;
    //This really is an infinite loop.
    while(true) {
        int num_events = epoll_wait(epoll_fd, events, EVENTS_LENGTH, -1);
        for(int i = 0; i < num_events; ++i) {

            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN))) {
                /* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
                fprintf (stderr, "epoll error\n");
                close (events[i].data.fd);
                continue;
            }
            else if(events[i].data.fd == server_socket_fd) {
                //There are new incoming connections to add to the epoll set
                while (true) {
                    //Grab the next incoming connection from the server socket
                    struct sockaddr in_addr;
                    socklen_t in_len = sizeof in_addr;
                    int incoming_fd = accept(server_socket_fd, &in_addr, &in_len);
                    if (incoming_fd == -1) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            /* We have processed all incoming connections. */
                            break;
                        } else {
                            perror("Error in accept");
                            break;
                        }
                    }

                    //For debugging only
//                    char host_buf[NI_MAXHOST], port_buf[NI_MAXSERV];
//                    int s = getnameinfo(&in_addr, in_len, host_buf, sizeof host_buf, port_buf, sizeof port_buf,
//                            NI_NUMERICHOST | NI_NUMERICSERV);
//                    if (s == 0) {
//                        printf("Accepted connection on descriptor %d "
//                                "(host=%s, port=%s)\n", incoming_fd, host_buf, port_buf);
//                    }
                    //Allocate a fresh buffer for the data sent by this sender
                    sender_buffers[incoming_fd] = std::vector<char>{};

                    //Make the incoming socket non-blocking and add it to the list of fds to monitor.,
                    int flags;
                    flags = fcntl(incoming_fd, F_GETFL, 0);
                    flags |= O_NONBLOCK;
                    if (fcntl(incoming_fd, F_SETFL, flags) < 0) {
                        throw connection_failure("Failed to set an incoming connection's socket to nonblocking!");
                    }

                    struct epoll_event event;
                    event.data.fd = incoming_fd;
                    event.events = EPOLLIN;
                    int success = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, incoming_fd, &event);
                    if (success == -1) {
                        perror("Error in epoll_ctl");
                        throw connection_failure("Failed to add a socket to the epoll set!");
                    }
                }
            }
            else {
                //We have data on a socket FD waiting to be read, so read it off
                bool done = false;
                while (true) {
                    char buf[512];

                    ssize_t count = read(events[i].data.fd, buf, sizeof buf);
                    if (count == -1) {
                        /* If errno == EAGAIN, that means we have read all
                         data. So we can break out of this loop. */
                        if (errno != EAGAIN) {
                            perror("read");
                            done = true;
                        }
                        break;
                    } else if (count == 0) {
                        /* End of file. The remote has closed the connection. */
                        done = true;
                        break;
                    }
                    //Append count bytes from buf to the buffer associated with this sender
                    sender_buffers[events[i].data.fd].insert(
                            sender_buffers[events[i].data.fd].end(), buf, (buf + count));

                }

                if (done) {
                    close(events[i].data.fd);
                    //Entire message has been received, so we can handle it
                    impl_this->receive_message(sender_buffers.at(events[i].data.fd));
                    sender_buffers.erase(events[i].data.fd);
                }
            }
        }
    }

    free(events);
}

} /* namespace networking */
} /* namespace pddm */
