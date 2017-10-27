/**
 * @file TcpUtilityClient.cpp
 *
 * @date Apr 13, 2017
 * @author edward
 */

#include "TcpUtilityClient.h"

#include <mutils-serialization/SerializationSupport.hpp>

#include "../messaging/AggregationMessage.h"
#include "../messaging/SignatureRequest.h"
#include "../messaging/SignatureResponse.h"
#include "../UtilityClient.h"


namespace pddm {
namespace networking {

TcpUtilityClient::TcpUtilityClient(UtilityClient& owning_utility_client, const TcpAddress& my_address,
        const std::map<int, TcpAddress>& meter_ips_by_id) :
            BaseTcpClient(this, my_address, meter_ips_by_id),
            logger(spdlog::get("global_logger")),
            utility_client(owning_utility_client) {}


void TcpUtilityClient::send(const std::shared_ptr<messaging::QueryRequest>& message, const int recipient_id) {
    Socket recipient_socket(id_to_ip_map.at(recipient_id).ip_addr, id_to_ip_map.at(recipient_id).port);
    bool success = true;
    auto bind_socket_write = [&](const char* bytes, std::size_t size) { success = recipient_socket.write(bytes, size) && success; };
    //Meter clients expect a "number of messages" first
    mutils::post_object(bind_socket_write, static_cast<std::size_t>(1));
    mutils::post_object(bind_socket_write, *message);
}

void TcpUtilityClient::send(const std::shared_ptr<messaging::SignatureResponse>& message, const int recipient_id) {
    Socket recipient_socket(id_to_ip_map.at(recipient_id).ip_addr, id_to_ip_map.at(recipient_id).port);
    bool success = true;
    auto bind_socket_write = [&](const char* bytes, std::size_t size) { success = recipient_socket.write(bytes, size) && success; };
    //Meter clients expect a "number of messages" first
    mutils::post_object(bind_socket_write, static_cast<std::size_t>(1));
    mutils::post_object(bind_socket_write, *message);
}

void TcpUtilityClient::receive_message(const std::vector<char>& message_bytes) {
    using namespace messaging;
    const char* buffer = message_bytes.data();
    /* This is the exact same logic used in Message::from_bytes. We could just do
     * auto message = mutils::from_bytes<messaging::Message>(nullptr, message_bytes.data());
     * but then we would have to use dynamic_pointer_cast to figure out which subclass
     * was deserialized and call the right utility_client.handle_message() overload.
     */
    MessageType message_type = *((MessageType*)(buffer));
    //Deserialize the correct message subclass based on the type, and call the correct handler
    switch(message_type) {
    case AggregationMessage::type: {
        std::shared_ptr<AggregationMessage> message{mutils::from_bytes<AggregationMessage>(nullptr, buffer)};
        utility_client.handle_message(message);
        break;
    }
    case SignatureRequest::type: {
        std::shared_ptr<SignatureRequest> message(mutils::from_bytes<SignatureRequest>(nullptr, buffer));
        utility_client.handle_message(message);
        break;
    }
    default:
        logger->warn("Utility dropped a message it didn't know how to handle!");
        break;
    }
}

std::function<TcpUtilityClient (UtilityClient&)> utility_network_client_builder(const TcpAddress& my_address,
        const std::map<int, TcpAddress>& meter_ips_by_id) {
    return [my_address, meter_ips_by_id](UtilityClient& utility_client) {
        return TcpUtilityClient(utility_client, my_address, meter_ips_by_id);
    };
}

} /* namespace networking */
} /* namespace pddm */
