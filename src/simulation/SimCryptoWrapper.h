/*
 * SimCrypto.h
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#pragma once

#include <functional>
#include <memory>
#include <string>

#include "../util/CryptoLibrary.h"
#include "SimCrypto.h"

namespace pddm {
class UtilityClient;
}

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
        SimCryptoWrapper(SimCrypto& wrapped_sim_crypto, const int calling_meter_id) :
            calling_meter_id(calling_meter_id), inner_sim_crypto(wrapped_sim_crypto) {}
        virtual ~SimCryptoWrapper() = default;

        std::shared_ptr<messaging::OverlayMessage> rsa_encrypt(
                const std::shared_ptr<messaging::OverlayMessage>& message, const int target_meter_id) override {
            return inner_sim_crypto.rsa_encrypt(calling_meter_id, message, target_meter_id);
        }
        std::shared_ptr<messaging::OverlayMessage> rsa_decrypt(
                const std::shared_ptr<messaging::OverlayMessage>& message) override {
            return inner_sim_crypto.rsa_decrypt(calling_meter_id, message);
        }
        std::shared_ptr<messaging::StringBody> rsa_encrypt(
                const std::shared_ptr<messaging::ValueTuple>& value, const int target_meter_id) override {
            return inner_sim_crypto.rsa_encrypt(calling_meter_id, value, target_meter_id);
        }
//        std::shared_ptr<messaging::ValueTuple> rsa_decrypt(
//                const std::shared_ptr<messaging::MessageBody>& value) override {
//            return inner_sim_crypto.rsa_decrypt(calling_meter_id, value);
//        }
        std::shared_ptr<messaging::StringBody> rsa_blind(
                const std::shared_ptr<messaging::ValueTuple>& value) override {
            return inner_sim_crypto.rsa_blind(calling_meter_id, value);
        }
        std::shared_ptr<messaging::StringBody> rsa_sign_blinded(const std::shared_ptr<messaging::StringBody>& encrypted_message) override {
            return inner_sim_crypto.rsa_sign_encrypted(calling_meter_id, encrypted_message);
        }
        void rsa_unblind_signature(const std::shared_ptr<std::string>& blinded_signature, util::SignatureArray& signature) override {
            return inner_sim_crypto.rsa_decrypt_signature(calling_meter_id, blinded_signature, signature);
        }
        void rsa_sign(const messaging::ValueContribution& value, util::SignatureArray& signature) override {
            inner_sim_crypto.rsa_sign(calling_meter_id, value, signature);
        }
        bool rsa_verify(const messaging::ValueContribution& value, const util::SignatureArray& signature,
                const int signer_meter_id) override {
            return inner_sim_crypto.rsa_verify(calling_meter_id, value, signature, signer_meter_id);
        }
        void rsa_sign(const messaging::SignedValue& value, util::SignatureArray& signature) override {
            return inner_sim_crypto.rsa_sign(calling_meter_id, value, signature);
        }
        bool rsa_verify(const messaging::SignedValue& value, const util::SignatureArray& signature,
                const int signer_meter_id) override {
            return inner_sim_crypto.rsa_verify(calling_meter_id, value, signature, signer_meter_id);
        }
};


std::function<SimCryptoWrapper (MeterClient&)> crypto_library_builder(SimCrypto& crypto_instance);
std::function<SimCryptoWrapper (UtilityClient&)> crypto_library_builder_utility(SimCrypto& crypto_instance);

} /* namespace simulation */
} /* namespace psm */

