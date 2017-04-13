/**
 * @file TcpUtilityClient.h
 *
 * @date Apr 13, 2017
 * @author edward
 */

#pragma once

#include <map>
#include <memory>
#include <spdlog/spdlog.h>

#include "BaseTcpClient.h"
#include "../UtilityNetworkClient.h"

namespace pddm {
class UtilityClient;
} /* namespace pddm */


namespace pddm {
namespace networking {

class TcpUtilityClient: public BaseTcpClient<TcpUtilityClient>, public UtilityNetworkClient {
        friend class BaseTcpClient;
    private:
        std::shared_ptr<spdlog::logger> logger;
        /** The UtilityClient that owns this TcpUtilityClient. */
        UtilityClient& utility_client;
    protected:
        void receive_message(const std::vector<char>& message_bytes);
    public:
        TcpUtilityClient(UtilityClient& owning_utility_client, const TcpAddress& my_address,
                const std::map<int, TcpAddress>& meter_ips_by_id);
        virtual ~TcpUtilityClient() = default;
        //Inherited from UtilityNetworkClient
        void send(const std::shared_ptr<messaging::QueryRequest>& message, const int recipient_id);
        void send(const std::shared_ptr<messaging::SignatureResponse>& message, const int recipient_id);

        using BaseTcpClient::monitor_incoming_messages;
};


std::function<TcpUtilityClient (UtilityClient&)> utility_network_client_builder(const TcpAddress& my_address,
        const std::map<int, TcpAddress>& meter_ips_by_id);

} /* namespace networking */
} /* namespace pddm */
