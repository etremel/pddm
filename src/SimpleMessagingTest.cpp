/**
 * @file SimpleMessagingTest.cpp
 *
 * @date Nov 7, 2016
 * @author edward
 */

#include <mutils-serialization/SerializationSupport.hpp>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "messaging/OverlayTransportMessage.h"
#include "messaging/PathOverlayMessage.h"
#include "messaging/ValueContribution.h"
#include "messaging/ValueTuple.h"
#include "networking/Socket.h"
#include "networking/ConnectionListener.h"
#include "networking/TcpAddress.h"

using namespace pddm;

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "Expected 2 arguments: sender/receiver mode, and IP:port for the server to use." << std::endl;
        exit(-1);
    }

    std::string mode(argv[1]);
    networking::TcpAddress server_ip = networking::parse_tcp_string(std::string(argv[2]));
    if(mode == "sender") {
        networking::Socket socket(server_ip.ip_addr, server_ip.port);
        bool success = true;
        auto bind_socket_write = [&](const char* bytes, std::size_t size) { success = socket.write(bytes, size) && success; };
        auto payload = std::make_shared<messaging::ValueContribution>(messaging::ValueTuple(2, {10}, {5, 15}));
        std::list<int> path = {1, 2, 3, 4};
        auto message = std::make_shared<messaging::PathOverlayMessage>(2, path, payload);
        messaging::OverlayTransportMessage wrapper(1, 3, true, message);
        std::size_t send_size = mutils::bytes_size(wrapper) + mutils::bytes_size(static_cast<std::size_t>(1));
        mutils::post_object(bind_socket_write, send_size);
        mutils::post_object(bind_socket_write, static_cast<std::size_t>(1));
        mutils::post_object(bind_socket_write, wrapper);
        std::cout << "Message sent!" << std::endl;
    } else if(mode == "receiver") {
        networking::ConnectionListener server_socket(server_ip.port);
        networking::Socket socket = server_socket.accept();
        std::size_t size;
        socket.read((char*) &size, sizeof(size));
        std::vector<char> message_bytes(size);
        socket.read(message_bytes.data(), size);
        std::cout << "Message received!" << std::endl;
        std::size_t num_messages = ((std::size_t*)(message_bytes.data()))[0];
        std::cout << "num_messages: " << num_messages << std::endl;
        const char* buffer = message_bytes.data() + sizeof(num_messages);
        messaging::MessageType message_type = *((messaging::MessageType*)(buffer));
        std::cout << "Message type is " << message_type << std::endl;
        auto message = mutils::from_bytes<messaging::OverlayTransportMessage>(nullptr, buffer);
        std::cout << *message << std::endl;
    }
}
