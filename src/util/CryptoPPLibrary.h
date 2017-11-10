/**
 * @file CryptoPPLibrary.h
 *
 * @date Oct 31, 2017
 * @author edward
 */

#pragma once

#include "CryptoLibrary.h"

#include "../messaging/OverlayMessage.h"
#include "../messaging/ValueTuple.h"
#include "../messaging/ValueContribution.h"
#include "../messaging/MessageBody.h"
#include "../messaging/StringBody.h"
#include "../messaging/SignedValue.h"

#include <cryptopp/rsa.h>
#include <cryptopp/randpool.h>
#include <cryptopp/osrng.h>

namespace pddm {

class MeterClient;
class UtilityClient;

namespace util {

class CryptoPPLibrary : public CryptoLibrary {
    private:
        CryptoPP::AutoSeededRandomPool rng;
        CryptoPP::RSA::PrivateKey my_private_key;
        CryptoPP::RSASSA_PKCS1v15_SHA_Signer my_rsa_signer;
        CryptoPP::RSAES_OAEP_SHA_Encryptor my_rsa_encryptor;
        CryptoPP::RSAES_OAEP_SHA_Decryptor my_rsa_decryptor;
        std::map<int, CryptoPP::RSA::PublicKey> public_keys_by_id;

        CryptoPP::RSA::PrivateKey load_private_key(const std::string& private_key_filename);

    public:
        CryptoPPLibrary(const std::string& private_key_filename, const std::map<int, std::string>& public_key_files_by_id);
        virtual ~CryptoPPLibrary() = default;

        std::shared_ptr<messaging::OverlayMessage> rsa_encrypt(
                const std::shared_ptr<messaging::OverlayMessage>& message, const int target_meter_id);

        std::shared_ptr<messaging::OverlayMessage> rsa_decrypt(
                const std::shared_ptr<messaging::OverlayMessage>& message);

        std::shared_ptr<messaging::StringBody> rsa_sign_encrypted(
                const std::shared_ptr<messaging::StringBody>& encrypted_message);

        void rsa_decrypt_signature(const std::shared_ptr<std::string>& blinded_signature,
                SignatureArray& signature);

        std::shared_ptr<messaging::StringBody> rsa_encrypt(
                const std::shared_ptr<messaging::ValueTuple>& value, const int target_meter_id);

        void rsa_sign(const messaging::ValueContribution& value, SignatureArray& signature);

        bool rsa_verify(const messaging::ValueContribution& value,
                const SignatureArray& signature, const int signer_meter_id);

        void rsa_sign(const messaging::SignedValue& value, SignatureArray& signature);

        bool rsa_verify(const messaging::SignedValue& value, const SignatureArray& signature,
                const int signer_meter_id);

};

std::function<CryptoPPLibrary (MeterClient&)> crypto_library_builder();
std::function<CryptoPPLibrary (UtilityClient&)> crypto_library_builder_utility();
} // namespace util


} // namespace pddm
