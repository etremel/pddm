/*
 * SimCrypto.h
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "../util/CryptoLibrary.h"
#include "SimNetworkClient.h"
#include "SimParameters.h"

namespace pddm {
namespace simulation {

class SimCrypto {
    private:
         /** Holds a reference to meter i's network client in index i. Must be
         * references (even though it's dangerous) because MeterClient stores its
         * NetworkClient by value. */
        std::vector<std::reference_wrapper<SimNetworkClient>> meter_network_clients;
    public:
        /** Constructs a SimCrypto instance to handle a simulation with the given
         * total number of meters. Only a single instance should be created per
         * simulation. */
        SimCrypto(const int num_meters) : meter_network_clients(num_meters) {}
        void add_meter(const int meter_id, const SimNetworkClient& meter_network_client);

        std::shared_ptr<messaging::OverlayMessage> rsa_encrypt_message(const int caller_id, const std::shared_ptr<messaging::OverlayMessage>& message) override;
        std::shared_ptr<messaging::OverlayMessage> rsa_decrypt_message(const int caller_id, const std::shared_ptr<messaging::OverlayMessage>& message) override;
        std::shared_ptr<messaging::MessageBody> rsa_encrypt_value(const std::shared_ptr<messaging::ValueContribution>& value) override;
        std::shared_ptr<messaging::ValueContribution> rsa_decrypt_value(const std::shared_ptr<messaging::MessageBody>& value) override;
        void rsa_sign_value(const int caller_id, const messaging::ValueTuple& value, uint8_t (&signature)[util::RSA_SIGNATURE_SIZE]) override;
        bool rsa_verify_value(const int caller_id, const messaging::ValueTuple& value, const uint8_t (&signature)[util::RSA_SIGNATURE_SIZE]) override;
        virtual ~SimCrypto();
};

} /* namespace simulation */
} /* namespace psm */
