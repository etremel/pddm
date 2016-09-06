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
#include "ConfigurationIncludes.h"

namespace pddm {
namespace messaging {
class AggregationMessage;
class OverlayTransportMessage;
class PingMessage;
class QueryRequest;
class SignatureResponse;
} /* namespace messaging */
} /* namespace pddm */

namespace pddm {

/**
 * Represents the client code that runs on a single meter; it reads measurements
 * from the meter in response to queries received over the network, and
 * communicates with other MeterClients during query processing. Most of the
 * logic of handling queries is inside an instance of ProtocolState_t.
 */
class MeterClient {
    public:
        const int meter_id;
        const int num_meters;
    private:
        Meter_t meter;
        NetworkClient_t network;
        CryptoLibrary_t crypto_library;
        TimerManager_t timer_library;

        int second_id;
        bool has_second_id;

        ProtocolState_t primary_protocol_state;
        std::experimental::optional<ProtocolState_t> secondary_protocol_state;

    public:
        MeterClient(const int id, const int num_meters, const MeterBuilderFunc& meter_builder, const NetworkClientBuilderFunc& network_builder,
                const CryptoLibraryBuilderFunc& crypto_library_builder, const TimerManagerBuilderFunc& timer_library_builder) :
                meter_id(id), num_meters(num_meters), meter(meter_builder(*this)), network(network_builder(*this)),
                crypto_library(crypto_library_builder(*this)), timer_library(timer_library_builder(*this)), second_id(0),
                has_second_id(false), primary_protocol_state(network, crypto_library, timer_library, num_meters, meter_id),
                secondary_protocol_state() {};
        MeterClient(MeterClient&&) = default;
        virtual ~MeterClient() = default;

        void set_second_id(const int id);

        /** Handles a message received from another meter or the utility.
         * This is a callback for NetworkClient to invoke when a message arrives from the network. */
        void handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message);
        /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
        void handle_message(const std::shared_ptr<messaging::AggregationMessage>& message);
        /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
        void handle_message(const std::shared_ptr<messaging::PingMessage>& message);
        /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
        void handle_message(const std::shared_ptr<messaging::QueryRequest>& message);
        /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
        void handle_message(const std::shared_ptr<messaging::SignatureResponse>& message);

        int get_num_meters() const { return num_meters; }
        //Obscene hack to allow Simulator to connect meters to the simulated Network. There's got to be a better way.
        NetworkClient_t& get_network_client() { return network; }

    private:
        //A pointer to ProtocolState_t will match exactly one of these, depending on which protocol is being used
        void handle_signature_response(const std::shared_ptr<messaging::SignatureResponse>& message, BftProtocolState* bft_protocol);
        void handle_signature_response(const std::shared_ptr<messaging::SignatureResponse>& message, void* protocol_is_not_bft);

};

}
