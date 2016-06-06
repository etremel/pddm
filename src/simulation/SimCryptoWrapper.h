/*
 * SimCrypto.h
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <cstdint>

#include "../util/CryptoLibrary.h"
#include "SimCrypto.h"

namespace pddm {
namespace simulation {

using util::RSA_STRENGTH;
using util::RSA_SIGNATURE_SIZE;

/**
 * Implementation of the CryptoLibrary interface that wraps a single global
 * instance of SimCrypto and forwards all calls to it. This allows each client
 * to have its own "CryptoLibrary object" when in the simulation they should
 * actually all be sharing the same object. It also allows the SimCrypto object
 * to learn which meter called its methods, so it can delay the right meter.
 */
class SimCryptoWrapper: public util::CryptoLibrary {
    private:
        int calling_meter_id;
        SimCrypto& inner_sim_crypto;
    public:
        SimCryptoWrapper(const SimCrypto& wrapped_sim_crypto, const int calling_meter_id) :
            calling_meter_id(calling_meter_id), inner_sim_crypto(wrapped_sim_crypto) {}
        virtual ~SimCryptoWrapper();

        std::shared_ptr<messaging::OverlayMessage> rsa_encrypt_message(const std::shared_ptr<messaging::OverlayMessage>& message) override {
            return inner_sim_crypto.rsa_encrypt_message(calling_meter_id, message);
        }
        std::shared_ptr<messaging::OverlayMessage> rsa_decrypt_message(const std::shared_ptr<messaging::OverlayMessage>& message) override {
            return inner_sim_crypto.rsa_decrypt_message(calling_meter_id, message);
        }
        std::shared_ptr<messaging::MessageBody> rsa_encrypt_value(const std::shared_ptr<messaging::ValueContribution>& value) override {
            return inner_sim_crypto.rsa_encrypt_value(calling_meter_id, value);
        }
        std::shared_ptr<messaging::ValueContribution> rsa_decrypt_value(const std::shared_ptr<messaging::MessageBody>& value) override {
            return inner_sim_crypto.rsa_decrypt_value(calling_meter_id, value);
        }
        void rsa_sign_value(const messaging::ValueTuple& value, util::SignatureArray& signature) override {
            inner_sim_crypto(calling_meter_id, value, signature);
        }
        bool rsa_verify_value(const messaging::ValueTuple& value, const util::SignatureArray& signature) override {
            return inner_sim_crypto(calling_meter_id, value, signature);
        }
};


CryptoLibraryBuilderFunc crypto_library_builder(SimCrypto& crypto_instance);
 std::function<CryptoLibrary_t (UtilityClient&)> crypto_library_builder_utility(SimCrypto& crypto_instance);

} /* namespace simulation */
} /* namespace psm */

