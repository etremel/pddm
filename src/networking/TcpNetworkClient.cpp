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
                utility_address(utility_address),
                num_messages_sent(0) {}

bool TcpNetworkClient::send(const std::list<std::shared_ptr<messaging::OverlayTransportMessage> >& messages, const int recipient_id) {
    Socket recipient_socket(id_to_ip_map.at(recipient_id).ip_addr, id_to_ip_map.at(recipient_id).port);
    bool success = true;
    auto bind_socket_write = [&](const char* bytes, std::size_t size) { success = recipient_socket.write(bytes, size) && success; };
    std::size_t bytes_sent = 0;
    mutils::post_object(bind_socket_write, messages.size());
    bytes_sent += mutils::bytes_size(messages.size());
    for(const auto& message : messages) {
        mutils::post_object(bind_socket_write, *message);
        bytes_sent += mutils::bytes_size(*message);
        num_messages_sent++;
    }
    return success;
}

bool TcpNetworkClient::send(const std::shared_ptr<messaging::AggregationMessage>& message, const int recipient_id) {
    //AggregationMessages might be sent to the utility, which has ID -1
    Socket recipient_socket;
    if(recipient_id == UTILITY_NODE_ID) {
        recipient_socket = Socket(utility_address.ip_addr, utility_address.port);
    } else {
        recipient_socket = Socket(id_to_ip_map.at(recipient_id).ip_addr, id_to_ip_map.at(recipient_id).port);
    }
    bool success = true;
    auto bind_socket_write = [&](const char* bytes, std::size_t size) { success = recipient_socket.write(bytes, size) && success; };
    //The utility doesn't need a "number of messages" header because it only accepts one message
    if(recipient_id != UTILITY_NODE_ID) {
        mutils::post_object(bind_socket_write, static_cast<std::size_t>(1));
    }
    mutils::post_object(bind_socket_write, *message);
    num_messages_sent++;
    return success;
}

bool TcpNetworkClient::send(const std::shared_ptr<messaging::PingMessage>& message, const int recipient_id) {
    Socket recipient_socket(id_to_ip_map.at(recipient_id).ip_addr, id_to_ip_map.at(recipient_id).port);
    bool success = true;
    auto bind_socket_write = [&](const char* bytes, std::size_t size) { success = recipient_socket.write(bytes, size) && success; };
    mutils::post_object(bind_socket_write, static_cast<std::size_t>(1));
    mutils::post_object(bind_socket_write, *message);
    num_messages_sent++;
    return success;
}

bool TcpNetworkClient::send(const std::shared_ptr<messaging::SignatureRequest>& message) {
    Socket utility_socket(utility_address.ip_addr, utility_address.port);
    bool success;
    auto bind_socket_write = [&](const char* bytes, std::size_t size) { success = utility_socket.write(bytes, size); };
    //No "number of messages" header for the utility
    mutils::post_object(bind_socket_write, *message);
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
