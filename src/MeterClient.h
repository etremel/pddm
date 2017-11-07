/*
 * MeterClient.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <experimental/optional>
#include <memory>
#include <spdlog/spdlog.h>

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
namespace simulation {
class Simulator;
}
}

namespace pddm {

/**
 * Represents the client code that runs on a single meter; it reads measurements
 * from the meter in response to queries received over the network, and
 * communicates with other MeterClients during query processing. Most of the
 * logic of handling queries is inside an instance of ProtocolState_t.
 */
class MeterClient {
    public:
        /** The ID of the meter in the data mining network. */
        const int meter_id;
        /** The total number of meters in the network; the meter client
         * must be re-initialized if this changes. */
        const int num_meters;
    private:
        std::shared_ptr<spdlog::logger> logger;
        /** A pointer to the meter interface this client should ask for measurements. */
        std::shared_ptr<Meter_t> meter;

        //FIXME: Just for deadline purposes, let the simulator reach in and access private members
        //(Actually, the MeterClient shouldn't have to know about the Simulator)
        friend class simulation::Simulator;

        /** The component of the meter client that manages network operations. */
        NetworkClient_t network_client;
        /** A stateful object encapsulating a cryptography library set up to
         * encrypt/decrypt with this meter's public key. */
        CryptoLibrary_t crypto_library;
        /** An instance of a timer library this meter client can use to set up timeout functions. */
        TimerManager_t timer_library;

    private:
        int second_id;
        bool has_second_id;

        ProtocolState_t primary_protocol_state;
        std::experimental::optional<ProtocolState_t> secondary_protocol_state;

    public:
        MeterClient(const int id, const int num_meters, const std::shared_ptr<Meter_t>& meter, const NetworkClientBuilderFunc& network_builder,
                const CryptoLibraryBuilderFunc& crypto_library_builder, const TimerManagerBuilderFunc& timer_library_builder) :
                    meter_id(id),
                    num_meters(num_meters),
                    logger(spdlog::get("global_logger")),
                    meter(meter),
                    network_client(network_builder(*this)),
                    crypto_library(crypto_library_builder(*this)),
                    timer_library(timer_library_builder(*this)),
                    second_id(0),
                    has_second_id(false),
                    primary_protocol_state(network_client, crypto_library, timer_library, num_meters, meter_id),
                    secondary_protocol_state() {};
        /** Moving a MeterClient will invalidate the references to it held in
         * network_client, crypto_library, and/or timer_library. */
        MeterClient(MeterClient&&) = delete;
        virtual ~MeterClient() = default;

        void set_second_id(const int id);

        /** Handles a message received from another meter or the utility.
         * This is a callback for NetworkClient to invoke when a message arrives from the network_client. */
        void handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message);
        /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
        void handle_message(const std::shared_ptr<messaging::AggregationMessage>& message);
        /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
        void handle_message(const std::shared_ptr<messaging::PingMessage>& message);
        /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
        void handle_message(const std::shared_ptr<messaging::QueryRequest>& message);
        /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
        void handle_message(const std::shared_ptr<messaging::SignatureResponse>& message);

        /** Starts the client, which will continuously wait for messages and
         * respond to them as they arrive. This function call never returns. */
        void main_loop();

        /** Shuts down the message-listening loop to allow the client to exit cleanly.
         * Obviously, this must be called from a separate thread from main_loop(). */
        void shut_down();

        int get_num_meters() const { return num_meters; }
        //Obscene hack to allow Simulator to connect meters to the simulated Network. There's got to be a better way.
        NetworkClient_t& get_network_client() { return network_client; }

    private:
        //A pointer to ProtocolState_t will match exactly one of these, depending on which protocol is being used
        void handle_signature_response(const std::shared_ptr<messaging::SignatureResponse>& message, BftProtocolState* bft_protocol);
        void handle_signature_response(const std::shared_ptr<messaging::SignatureResponse>& message, void* protocol_is_not_bft);

};

}
