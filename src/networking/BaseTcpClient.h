/**
 * @file BaseTcpClient.h
 *
 * @date Apr 13, 2017
 * @author edward
 */

#pragma once

#include <atomic>
#include <map>
#include <vector>

#include "TcpAddress.h"

namespace pddm {
namespace networking {

class Socket;

/**
 * Contains the common implementation details of TcpNetworkClient and
 * TcpUtilityClient, such as the epoll monitoring loop for receiving incoming
 * messages. This class should not be used polymorphically and uses the CRTP to
 * avoid virtual function dispatch when handing off to the subclass's
 * implementation of receive_message().
 */
template<typename Impl>
class BaseTcpClient {
    private:
        Impl* impl_this;
        void require_receive_message(const std::vector<char>& message_bytes) { impl_this->receive_message(message_bytes); }
    protected:
        /** Maps Meter IDs to IP address/port pairs. */
        std::map<int, TcpAddress> id_to_ip_map;
        /** Cache of open sockets to meters, lazily initialized (the socket
         * is created the first time a message is sent to that meter). This
         * may also contain a socket for the utility, at entry -1. */
        std::map<int, Socket> sockets_by_id;
    private:
        /** File descriptor for monitoring incoming connections */
        int epoll_fd;
        int server_socket_fd;
        std::atomic<bool> shutdown;
    protected:
        BaseTcpClient(Impl* subclass_this, const TcpAddress& my_address, const std::map<int, TcpAddress>& meter_ips_by_id);
        virtual ~BaseTcpClient();
    public:
        /**
         * Loops forever, waiting for incoming connections and calling the subclass's
         * receive_message() when a connected client sends a complete message.
         * Calls to this function will not return, so client applications must
         * dedicate a thread to it.
         */
        void monitor_incoming_messages();
        /**
         * Shuts down the monitor_incoming_messages() loop. Can be called from a
         * different thread to allow the monitor_incoming_messages() thread to
         * terminate.
         */
        void shut_down();
};

} /* namespace networking */
} /* namespace pddm */

#include "BaseTcpClient_impl.h"
