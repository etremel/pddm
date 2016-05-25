/*
 * CryptoLibrary.h
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include "../messaging/OverlayMessage.h"
#include "../messaging/ValueContribution.h"

namespace pddm {

namespace util {

constexpr int RSA_STRENGTH = 2048;
constexpr int RSA_SIGNATURE_SIZE = RSA_STRENGTH / 8;

/**
 * The interface for all cryptography functions needed by the meter client protocols.
 * This is a class rather than a set of functions because some cryptography libraries
 * need to maintain state between successive function invocations. (The simulation
 * implementation of this won't actually do any cryptography).
 */
class CryptoLibrary {
    public:
        virtual std::shared_ptr<messaging::OverlayMessage> rsa_encrypt_message(const std::shared_ptr<messaging::OverlayMessage>& message);
        virtual std::shared_ptr<messaging::OverlayMessage> rsa_decrypt_message(const std::shared_ptr<messaging::OverlayMessage>& message);
        virtual std::shared_ptr<messaging::MessageBody> rsa_encrypt_value(const std::shared_ptr<messaging::ValueContribution>& value);
        virtual std::shared_ptr<messaging::ValueContribution> rsa_decrypt_value(const std::shared_ptr<messaging::MessageBody>& value);
        virtual void rsa_sign_value(const messaging::ValueTuple& value, uint8_t (&signature)[RSA_SIGNATURE_SIZE]);
        virtual bool rsa_verify_value(const messaging::ValueTuple& value, const uint8_t (&signature)[RSA_SIGNATURE_SIZE]);
        virtual ~CryptoLibrary() = 0;
};

}
}


