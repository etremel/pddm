/**
 * @file TcpNetworkClient.cpp
 *
 * @date Oct 12, 2016
 * @author edward
 */

#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <mutils-serialization/SerializationSupport.hpp>

#include "TcpNetworkClient.h"
#include "Socket.h"
#include "../MeterClient.h"
#include "../messaging/QueryRequest.h"

namespace pddm {
namespace networking {

TcpNetworkClient::TcpNetworkClient(MeterClient& owning_meter_client, const TcpAddress& my_address,
        const TcpAddress& utility_address, const std::map<int, TcpAddress>& meter_ips_by_id) :
                BaseTcpClient(this, my_address, meter_ips_by_id),
                logger(spdlog::get("global_logger")),
                meter_client(owning_meter_client),
                num_messages_sent(0) {
    sockets_by_id.emplace(UTILITY_NODE_ID, Socket(utility_address.ip_addr, utility_address.port));
}

bool TcpNetworkClient::send(const std::list<std::shared_ptr<messaging::OverlayTransportMessage> >& messages, const int recipient_id) {
    //Construct a new socket for this node if there is not one already in the map
    auto socket_map_find = sockets_by_id.lower_bound(recipient_id);
    if(socket_map_find == sockets_by_id.end() || socket_map_find->first != recipient_id) {
        sockets_by_id.emplace_hint(socket_map_find, recipient_id,
                Socket(id_to_ip_map.at(recipient_id).ip_addr, id_to_ip_map.at(recipient_id).port));
    }
    std::size_t send_size = mutils::bytes_size(messages.size());
    for(const auto& message : messages) {
        send_size += mutils::bytes_size(*message);
    }
    char buffer[send_size + sizeof(send_size)];
    std::memcpy(buffer, (char*)&send_size, sizeof(send_size));
    std::size_t bytes_written = sizeof(send_size);
    bytes_written += mutils::to_bytes(messages.size(), buffer + bytes_written);
    for(const auto& message : messages) {
        bytes_written += mutils::to_bytes(*message, buffer + bytes_written);
        num_messages_sent++;
    }
    bool success = sockets_by_id.at(recipient_id).write(buffer, send_size + sizeof(send_size));
    return success;
}

bool TcpNetworkClient::send(const std::shared_ptr<messaging::AggregationMessage>& message, const int recipient_id) {
    //Construct a new socket for this node if there is not one already in the map
    auto socket_map_find = sockets_by_id.lower_bound(recipient_id);
    if(socket_map_find == sockets_by_id.end() || socket_map_find->first != recipient_id) {
        sockets_by_id.emplace_hint(socket_map_find, recipient_id,
                Socket(id_to_ip_map.at(recipient_id).ip_addr, id_to_ip_map.at(recipient_id).port));
    }
    const std::size_t num_messages = 1;
    std::size_t send_size = mutils::bytes_size(*message);
    //The utility doesn't need a "number of messages" header because it only accepts one message
    if(recipient_id != UTILITY_NODE_ID) {
        send_size += mutils::bytes_size(num_messages);
    }
    char buffer[send_size + sizeof(send_size)];
    std::memcpy(buffer, (char*)&send_size, sizeof(send_size));
    std::size_t bytes_written = sizeof(send_size);
    if(recipient_id != UTILITY_NODE_ID) {
        bytes_written += mutils::to_bytes(num_messages, buffer + bytes_written);
    }
    bytes_written += mutils::to_bytes(*message, buffer + bytes_written);
    bool success = sockets_by_id.at(recipient_id).write(buffer, send_size + sizeof(send_size));
    num_messages_sent++;
    return success;
}

bool TcpNetworkClient::send(const std::shared_ptr<messaging::PingMessage>& message, const int recipient_id) {
    auto socket_map_find = sockets_by_id.lower_bound(recipient_id);
    if(socket_map_find == sockets_by_id.end() || socket_map_find->first != recipient_id) {
        sockets_by_id.emplace_hint(socket_map_find, recipient_id,
                Socket(id_to_ip_map.at(recipient_id).ip_addr, id_to_ip_map.at(recipient_id).port));
    }    const std::size_t num_messages = 1;
    std::size_t send_size = mutils::bytes_size(num_messages) + mutils::bytes_size(*message);
    char buffer[send_size + sizeof(send_size)];
    std::memcpy(buffer, (char*)&send_size, sizeof(send_size));
    std::size_t bytes_written = sizeof(send_size);
    bytes_written += mutils::to_bytes(num_messages, buffer + bytes_written);
    bytes_written += mutils::to_bytes(*message, buffer + bytes_written);
    bool success = sockets_by_id.at(recipient_id).write(buffer, send_size + sizeof(send_size));
    num_messages_sent++;
    return success;
}

bool TcpNetworkClient::send(const std::shared_ptr<messaging::SignatureRequest>& message) {
    //No "number of messages" header for the utility
    std::size_t send_size = mutils::bytes_size(*message);
    char buffer[send_size + sizeof(send_size)];
    std::memcpy(buffer, (char*)&send_size, sizeof(send_size));
    mutils::to_bytes(*message, buffer + sizeof(send_size));
    bool success = sockets_by_id.at(UTILITY_NODE_ID).write(buffer, send_size + sizeof(send_size));
    num_messages_sent++;
    return success;
}

void TcpNetworkClient::receive_message(const std::vector<char>& message_bytes) {
    using namespace messaging;
    //First, read the number of messages in the list
    std::size_t num_messages = ((std::size_t*) message_bytes.data())[0];
    const char* buffer = message_bytes.data() + sizeof(num_messages);
    //Deserialize that number of messages, moving the buffer pointer each time one is deserialized
    for(auto i = 0u; i < num_messages; ++i) {
        /* This is the exact same logic used in Message::from_bytes. We could just do
         * auto message = mutils::from_bytes<messaging::Message>(nullptr, message_bytes.data());
         * but then we would have to use dynamic_pointer_cast to figure out which subclass
         * was deserialized and call the right meter_client.handle_message() overload.
         */
        MessageType message_type = ((MessageType*)(buffer))[0];
        //Deserialize the correct message subclass based on the type, and call the correct handler
        switch(message_type) {
        case OverlayTransportMessage::type: {
            std::shared_ptr<OverlayTransportMessage> message(mutils::from_bytes<OverlayTransportMessage>(nullptr, buffer));
            buffer += mutils::bytes_size(*message);
            meter_client.handle_message(message);
            break;
        }
        case PingMessage::type: {
            std::shared_ptr<PingMessage> message(mutils::from_bytes<PingMessage>(nullptr, buffer));
            buffer += mutils::bytes_size(*message);
            meter_client.handle_message(message);
            break;
        }
        case AggregationMessage::type: {
            std::shared_ptr<AggregationMessage> message(mutils::from_bytes<AggregationMessage>(nullptr, buffer));
            buffer += mutils::bytes_size(*message);
            meter_client.handle_message(message);
            break;
        }
        case QueryRequest::type: {
            std::shared_ptr<QueryRequest> message(mutils::from_bytes<QueryRequest>(nullptr, buffer));
            buffer += mutils::bytes_size(*message);
            std::cout << "Received a QueryRequest: " << *message << std::endl;
            meter_client.handle_message(message);
            break;
        }
        case SignatureResponse::type: {
            std::shared_ptr<SignatureResponse> message(mutils::from_bytes<SignatureResponse>(nullptr, buffer));
            buffer += mutils::bytes_size(*message);
            meter_client.handle_message(message);
            break;
        }
        default:
            logger->warn("Meter {} dropped a message it didn't know how to handle.", meter_client.meter_id);
            break;
        }
    }
}

std::function<TcpNetworkClient(MeterClient&)> network_client_builder(const TcpAddress& my_address,
        const TcpAddress& utility_address, const std::map<int, TcpAddress>& meter_ips_by_id) {
    return [utility_address, my_address, meter_ips_by_id](MeterClient& meter_client) {
        return TcpNetworkClient(meter_client, my_address, utility_address, meter_ips_by_id);
    };
}


} /* namespace networking */
} /* namespace pddm */
