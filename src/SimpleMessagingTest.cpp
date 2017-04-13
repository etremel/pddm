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

#include "messaging/OverlayMessage.h"
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
        auto payload = std::make_shared<messaging::ValueContribution>(messaging::ValueTuple(2, {10}, {5, 15}));
        std::list<int> path = {1, 2, 3, 4};
        messaging::PathOverlayMessage message(2, path, payload);
        std::size_t size = mutils::bytes_size(message);
        char buffer[size];
        mutils::to_bytes(message, buffer);
        socket.write((char*) &size, sizeof(size));
        socket.write(buffer, size);
        std::cout << "Message sent!" << std::endl;
    } else if(mode == "receiver") {
        networking::ConnectionListener server_socket(server_ip.port);
        networking::Socket socket = server_socket.accept();
        std::size_t size;
        socket.read((char*) &size, sizeof(size));
        char buffer[size];
        socket.read(buffer, size);
        std::cout << "Message received!" << std::endl;
        auto message = mutils::from_bytes<messaging::PathOverlayMessage>(nullptr, buffer);
        std::cout << *message << std::endl;
    }
}
