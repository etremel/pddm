/**
 * @file TcpUtilityClient.cpp
 *
 * @date Apr 13, 2017
 * @author edward
 */

#include "TcpUtilityClient.h"

#include <mutils-serialization/SerializationSupport.hpp>

#include "Socket.h"
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
    //Construct a new socket for this node if there is not one already in the map
    auto socket_map_find = sockets_by_id.lower_bound(recipient_id);
    if(socket_map_find == sockets_by_id.end() || socket_map_find->first != recipient_id) {
        sockets_by_id.emplace_hint(socket_map_find, recipient_id,
                Socket(id_to_ip_map.at(recipient_id).ip_addr, id_to_ip_map.at(recipient_id).port));
    }
    //Meter clients expect a "number of messages" first
    const std::size_t num_messages = 1;
    std::size_t send_size = mutils::bytes_size(num_messages) + mutils::bytes_size(*message);
    char buffer[send_size + sizeof(send_size)];
    std::memcpy(buffer, (char*)&send_size, sizeof(send_size));
    std::size_t bytes_written = sizeof(send_size);
    bytes_written += mutils::to_bytes(num_messages, buffer + bytes_written);
    bytes_written += mutils::to_bytes(*message, buffer + bytes_written);
    sockets_by_id.at(recipient_id).write(buffer, send_size + sizeof(send_size));
}

void TcpUtilityClient::send(const std::shared_ptr<messaging::SignatureResponse>& message, const int recipient_id) {
    //Exactly the same as the other send(), but must be re-implemented becuase the message is a different type
    auto socket_map_find = sockets_by_id.lower_bound(recipient_id);
    if(socket_map_find == sockets_by_id.end() || socket_map_find->first != recipient_id) {
        sockets_by_id.emplace_hint(socket_map_find, recipient_id,
                Socket(id_to_ip_map.at(recipient_id).ip_addr, id_to_ip_map.at(recipient_id).port));
    }
    const std::size_t num_messages = 1;
    std::size_t send_size = mutils::bytes_size(num_messages) + mutils::bytes_size(*message);
    char buffer[send_size + sizeof(send_size)];
    std::memcpy(buffer, (char*)&send_size, sizeof(send_size));
    std::size_t bytes_written = sizeof(send_size);
    bytes_written += mutils::to_bytes(num_messages, buffer + bytes_written);
    bytes_written += mutils::to_bytes(*message, buffer + bytes_written);
    sockets_by_id.at(recipient_id).write(buffer, send_size + sizeof(send_size));
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
