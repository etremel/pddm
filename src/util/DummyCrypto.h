/**
 * @file DummyCrypto.h
 *
 * @date Apr 12, 2017
 * @author edward
 */

#pragma once

#include <sstream>

#include "CryptoLibrary.h"

#include "../messaging/OverlayMessage.h"
#include "../messaging/ValueTuple.h"
#include "../messaging/ValueContribution.h"
#include "../messaging/MessageBody.h"
#include "../messaging/StringBody.h"
#include "../messaging/SignedValue.h"

namespace pddm {
class UtilityClient;

namespace util {

/**
 * A "dummy" implementation of the cryptography library that implements every
 * operation as a no-op. Use this implementation to completely disable cryptography.
 */
class DummyCrypto : public CryptoLibrary {
    public:
        virtual ~DummyCrypto() = default;

        std::shared_ptr<messaging::OverlayMessage> rsa_encrypt(
                const std::shared_ptr<messaging::OverlayMessage>& message, const int target_meter_id) override {
            message->is_encrypted = true;
            return message;
        }

        std::shared_ptr<messaging::OverlayMessage> rsa_decrypt(
                const std::shared_ptr<messaging::OverlayMessage>& message) override {
            message->is_encrypted = false;
            return message;
        }

        std::shared_ptr<messaging::StringBody> rsa_encrypt(
                const std::shared_ptr<messaging::ValueTuple>& value, const int target_meter_id) override {
            std::stringstream stringifier;
            stringifier << *value;
            return std::make_shared<messaging::StringBody>(stringifier.str());
        }

        std::shared_ptr<messaging::StringBody> rsa_sign_encrypted(
                const std::shared_ptr<messaging::StringBody>& encrypted_message) override {
            return encrypted_message;
        }
        void rsa_decrypt_signature(const std::shared_ptr<std::string>& blinded_signature,
                util::SignatureArray& signature) override {
            signature.fill(0);
        }
        void rsa_sign(const messaging::ValueContribution& value, util::SignatureArray& signature) override {
            signature.fill(0);
        }

        bool rsa_verify(const messaging::ValueContribution& value, const util::SignatureArray& signature,
                const int signer_meter_id) override {
            return true;
        }

        void rsa_sign(const messaging::SignedValue& value, util::SignatureArray& signature) override {
            signature.fill(0);
        }

        bool rsa_verify(const messaging::SignedValue& value, const util::SignatureArray& signature,
                const int signer_meter_id) override {
            return true;
        }
};

inline std::function<DummyCrypto (MeterClient&)> crypto_library_builder() {
    return [](MeterClient& client) { return DummyCrypto(); };
}
inline std::function<DummyCrypto (UtilityClient&)> crypto_library_builder_utility() {
    return [](UtilityClient& client) { return DummyCrypto(); };
}


}
}


