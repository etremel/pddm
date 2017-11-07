/**
 * @file BaseTcpClient.cpp
 *
 * @date Apr 13, 2017
 * @author edward
 */

#include <cstring>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <iostream>

#include "BaseTcpClient.h"
#include "Socket.h"

namespace pddm {
namespace networking {

template<typename Impl>
BaseTcpClient<Impl>::BaseTcpClient(Impl* subclass_this, const TcpAddress& my_address,
        const std::map<int, TcpAddress>& meter_ips_by_id) :
        impl_this(subclass_this),
        id_to_ip_map(meter_ips_by_id),
        shutdown(false) {
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
    memset(&event, 0, sizeof(event));
    event.data.fd = server_socket_fd;
    event.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket_fd, &event);
}

template<typename Impl>
BaseTcpClient<Impl>::~BaseTcpClient() {
    shutdown = true;
    close(server_socket_fd);
}

template<typename Impl>
void BaseTcpClient<Impl>::shut_down() {
    shutdown = true;
    //Open and close a connection to myself to make epoll_wait wake up and do nothing
    sockaddr_in my_address;
    socklen_t size_of_sockaddr = sizeof(my_address);
    memset(&my_address, 0, sizeof(my_address));
    getsockname(server_socket_fd, (struct sockaddr*) &my_address, &size_of_sockaddr);
    Socket dummy{"127.0.0.1", my_address.sin_port};
    dummy.write("", 0);
}

template<typename Impl>
void BaseTcpClient<Impl>::monitor_incoming_messages() {
    const int EVENTS_LENGTH = 64;
    struct epoll_event* events = (struct epoll_event*) calloc(EVENTS_LENGTH, sizeof(struct epoll_event));
    //Indexed by FD, since that's all the information we get when a socket has data
    std::map<int, std::vector<char>> sender_buffers;
    while(!shutdown) {
        int num_events = epoll_wait(epoll_fd, events, EVENTS_LENGTH, 100);
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
                    sender_buffers.emplace(incoming_fd, std::vector<char>{});

                    //Make the incoming socket non-blocking and add it to the list of fds to monitor.,
                    int flags;
                    flags = fcntl(incoming_fd, F_GETFL, 0);
                    flags |= O_NONBLOCK;
                    if (fcntl(incoming_fd, F_SETFL, flags) < 0) {
                        throw connection_failure("Failed to set an incoming connection's socket to nonblocking!");
                    }

                    struct epoll_event event;
                    memset(&event, 0, sizeof(event));
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
                /* In all PDDM messaging protocols, the first 4 bytes are a size_t
                 * containing the size of the rest of the message */
                std::size_t message_size;
                ssize_t int_bytes_read = recv(events[i].data.fd, (char*)&message_size, sizeof(message_size), MSG_WAITALL);
                if(int_bytes_read == 0) {
                    //If the very first read is zero, the client has closed the connection
                    close(events[i].data.fd);
                    continue;
                } else if(int_bytes_read != sizeof(message_size)) {
                    perror("Failed to read the first 4 bytes of an incoming message!");
                    continue;
                }
                sender_buffers[events[i].data.fd].resize(message_size);
                /* Block until the entire message has been read - the sender should have
                 * packed it into one send, so this won't block for very long. */
                ssize_t msg_bytes_read = recv(events[i].data.fd, sender_buffers[events[i].data.fd].data(), message_size, MSG_WAITALL);
                if(msg_bytes_read != message_size) {
                    //Client must have failed or disconnected
                    perror("Failure while reading a message from a client");
                    sender_buffers.erase(events[i].data.fd);
                    continue;
                }
                //Handle the message
                impl_this->receive_message(sender_buffers.at(events[i].data.fd));
                sender_buffers.erase(events[i].data.fd);
            }
        }
    }

    free(events);
}

} /* namespace networking */
} /* namespace pddm */
