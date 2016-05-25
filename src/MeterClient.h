/*
 * MeterClient.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <experimental/optional>
#include <memory>

#include "Configuration.h"
#include "simulation/Network.h"

namespace pddm {

class MeterClient {
    public:
        const int meter_id;
    private:
        Meter_t meter;
        NetworkClient_t network;
        CryptoLibrary_t crypto_library;
        TimerManager_t timer_library;

        int second_id;
        bool has_second_id;
        const int num_meters;

        ProtocolState_t primary_protocol_state;
        std::experimental::optional<ProtocolState_t> secondary_protocol_state;

    public:
        MeterClient(const int id, const int num_meters, const MeterBuilderFunc& meter_builder, const NetworkClientBuilderFunc& network_builder,
                const CryptoLibraryBuilderFunc& crypto_library_builder, const TimerManagerBuilderFunc& timer_library_builder) :
            meter_id(id), meter(meter_builder(*this)), network(network_builder(*this)), crypto_library(crypto_library_builder(*this)),
            timer_library(timer_library_builder(*this)), second_id(0), has_second_id(false), num_meters(num_meters),
            primary_protocol_state(network, crypto_library, timer_library, *this, meter_id),
            secondary_protocol_state(std::experimental::nullopt_t) {};
        virtual ~MeterClient();

        void set_second_id(const int id);

        /** Handles a message received from another meter or the utility.
         * This is a callback for NetworkClient to invoke when a message arrives from the network. */
        void handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message);
        void handle_message(const std::shared_ptr<messaging::AggregationMessage>& message);
        void handle_message(const std::shared_ptr<messaging::PingMessage>& message);
        void handle_message(const std::shared_ptr<messaging::QueryRequest>& message);
        void handle_message(const std::shared_ptr<messaging::SignatureResponse>& message);

        int get_num_meters() const { return num_meters; }


};

}
