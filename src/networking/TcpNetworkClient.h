/**
 * @file TcpNetworkClient.h
 *
 * @date Oct 12, 2016
 * @author edward
 */

#pragma once

#include <map>
#include <memory>
#include <spdlog/spdlog.h>

#include "../NetworkClient.h"
#include "BaseTcpClient.h"

namespace pddm {
class MeterClient;
} /* namespace pddm */

namespace pddm {
namespace networking {

class Socket;
class ConnectionListener;


class TcpNetworkClient : public BaseTcpClient<TcpNetworkClient>, public NetworkClient {
        friend class BaseTcpClient;
    private:
        std::shared_ptr<spdlog::logger> logger;
        /** The MeterClient that owns this TCPNetworkClient */
        MeterClient& meter_client;
        TcpAddress utility_address;
        /** Message count tracker for experiment graphs. */
        int num_messages_sent;
    protected:
        void receive_message(const std::vector<char>& message_bytes);
    public:
        /**
         * Constructs a NetworkClient for sending over TCP networks using Linux
         * sockets. For now, the list of all meter IP addresses/ports must be
         * statically configured and set at startup (eventually, it would be
         * nice to provide a way for the utility/server to dynamically configure
         * clients)
         * @param owning_meter_client The MeterClient that this NetworkClient
         * is a component of
         * @param my_address The IP address of the node running this NetworkClient
         * @param utility_address The IP address of the central utility server
         * @param meter_ips_by_id A map of the IP addresses of all the other
         * client nodes in the system, indexed by their meter ID.
         */
        TcpNetworkClient(MeterClient& owning_meter_client, const TcpAddress& my_address,
                const TcpAddress& utility_address, const std::map<int, TcpAddress>& meter_ips_by_id);
        virtual ~TcpNetworkClient() = default;
        //Inherited from NetworkClient
        bool send(const std::list<std::shared_ptr<messaging::OverlayTransportMessage>>& messages, const int recipient_id);
        bool send(const std::shared_ptr<messaging::AggregationMessage>& message, const int recipient_id);
        bool send(const std::shared_ptr<messaging::PingMessage>& message, const int recipient_id);
        bool send(const std::shared_ptr<messaging::SignatureRequest>& message);
        /* Stupid indirection just to get the compiler to believe that BaseTcpClient provides
         * an implementation of monitor_incoming_messages to satisfy NetworkClient's interface */
        inline void monitor_incoming_messages() {
            BaseTcpClient::monitor_incoming_messages();
        }

};

std::function<TcpNetworkClient (MeterClient&)> network_client_builder(const TcpAddress& my_address,
        const TcpAddress& utility_address, const std::map<int, TcpAddress>& meter_ips_by_id);

} /* namespace networking */
} /* namespace pddm */

