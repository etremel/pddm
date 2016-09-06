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
#include <string>
#include <map>

#include "../util/CryptoLibrary.h"
#include "SimNetworkClient.h"
#include "SimParameters.h"

namespace pddm {
namespace simulation {

/**
 * This class manages simulated cryptography by causing clients to get delayed
 * (in simulated time) when they call cryptography functions. Every simulated
 * MeterClient has a SimCryptoWrapper that references a single "global" instance
 * of this class.
 */
class SimCrypto {
    private:
         /** Holds a reference to meter i's network client in index i. Must be
         * references (even though it's dangerous) because MeterClient stores its
         * NetworkClient by value. */
        std::vector<std::reference_wrapper<SimNetworkClient>> meter_network_clients;
        std::map<int, std::reference_wrapper<SimNetworkClient>> meter_network_clients_setup;
    public:
        /** Constructs a SimCrypto instance to handle a simulation with the given
         * total number of meters. Only a single instance should be created per
         * simulation. */
        SimCrypto(const int num_meters) {}
        virtual ~SimCrypto() = default;
        void add_meter(const int meter_id, SimNetworkClient& meter_network_client);
        void finish_setup();

        std::shared_ptr<messaging::OverlayMessage> rsa_encrypt(const int caller_id,
                const std::shared_ptr<messaging::OverlayMessage>& message, const int target_meter_id);
        std::shared_ptr<messaging::OverlayMessage> rsa_decrypt(const int caller_id,
                const std::shared_ptr<messaging::OverlayMessage>& message);
        std::shared_ptr<messaging::StringBody> rsa_encrypt(const int caller_id,
                const std::shared_ptr<messaging::ValueTuple>& value, const int target_meter_id);
//        std::shared_ptr<messaging::ValueTuple> rsa_decrypt(const int caller_id,
//                const std::shared_ptr<messaging::MessageBody>& value);
        std::shared_ptr<messaging::StringBody> rsa_sign_encrypted(const int caller_id,
                const std::shared_ptr<messaging::StringBody>& encrypted_message);
        void rsa_decrypt_signature(const int caller_id, const std::shared_ptr<std::string>& blinded_signature,
                util::SignatureArray& signature);
        void rsa_sign(const int caller_id, const messaging::ValueContribution& value,
                util::SignatureArray& signature);
        bool rsa_verify(const int caller_id, const messaging::ValueContribution& value,
                const util::SignatureArray& signature, const int signer_meter_id);
        void rsa_sign(const int caller_id, const messaging::SignedValue& value, util::SignatureArray& signature);
        bool rsa_verify(const int caller_id, const messaging::SignedValue& value, const util::SignatureArray& signature,
                const int signer_meter_id);
};

} /* namespace simulation */
} /* namespace psm */
