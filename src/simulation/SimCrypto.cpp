/*
 * SimCrypto.cpp
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#include "SimCrypto.h"

#include <vector>
#include <functional>
#include <memory>
#include <string>

#include "../messaging/OverlayMessage.h"
#include "../messaging/ValueTuple.h"
#include "../messaging/ValueContribution.h"
#include "../messaging/MessageBody.h"
#include "../messaging/StringBody.h"
#include "../messaging/SignedValue.h"

namespace pddm {
namespace simulation {

void SimCrypto::add_meter(const int meter_id, SimNetworkClient& meter_network_client) {
    meter_network_clients_setup.emplace(meter_id, std::ref(meter_network_client));
}


void SimCrypto::finish_setup() {
    if(!meter_network_clients.empty())
        throw std::runtime_error("finish_setup called more than once!");
    if(meter_network_clients_setup.empty())
        throw std::runtime_error("No clients were added to SimCrypto!");
    const int last_id = meter_network_clients_setup.rbegin()->first;
    for(int id = 0; id <= last_id; ++id) {
        meter_network_clients.emplace_back(meter_network_clients_setup.at(id));
    }
    meter_network_clients_setup.clear();
}

std::shared_ptr<messaging::OverlayMessage> SimCrypto::rsa_encrypt(const int caller_id,
        const std::shared_ptr<messaging::OverlayMessage>& message, const int target_meter_id) {
    //The utility has id -1, and doesn't get delayed because it's a datacenter
    if(caller_id > -1) meter_network_clients.at(caller_id).get().delay_client(RSA_ENCRYPT_TIME_MICROS);
    message->is_encrypted = true;
    return message;
}

std::shared_ptr<messaging::OverlayMessage> SimCrypto::rsa_decrypt(const int caller_id,
        const std::shared_ptr<messaging::OverlayMessage>& message) {
    if(caller_id > -1) meter_network_clients.at(caller_id).get().delay_client(RSA_DECRYPT_TIME_MICROS);
    message->is_encrypted = false;
    return message;
}

void SimCrypto::rsa_sign(const int caller_id, const messaging::ValueContribution& value,
        util::SignatureArray& signature) {
    if(caller_id > -1) meter_network_clients.at(caller_id).get().delay_client(RSA_SIGN_TIME_MICROS);
}

bool SimCrypto::rsa_verify(const int caller_id, const messaging::ValueContribution& value,
        const util::SignatureArray& signature, const int signer_meter_id) {
    if(caller_id > -1) meter_network_clients.at(caller_id).get().delay_client(RSA_VERIFY_TIME_MICROS);
    return true;
}

std::shared_ptr<messaging::StringBody> SimCrypto::rsa_encrypt(const int caller_id,
        const std::shared_ptr<messaging::ValueTuple>& value, const int target_meter_id) {
    if(caller_id > -1) meter_network_clients.at(caller_id).get().delay_client(RSA_ENCRYPT_TIME_MICROS);
    return std::make_shared<messaging::StringBody>();
}

std::shared_ptr<messaging::StringBody> SimCrypto::rsa_blind(const int caller_id,
                const std::shared_ptr<messaging::ValueTuple>& value) {
    //"Blinding" is a multiplication and exponentiation using the utility's public key, so it's basically encryption
    if(caller_id > -1) meter_network_clients.at(caller_id).get().delay_client(RSA_ENCRYPT_TIME_MICROS);
    return std::make_shared<messaging::StringBody>();
}

//std::shared_ptr<messaging::ValueTuple> SimCrypto::rsa_decrypt(const int caller_id,
//        const std::shared_ptr<messaging::MessageBody>& value) {
//    if(caller_id > -1) meter_network_clients.at(caller_id).get().delay_client(RSA_DECRYPT_TIME_MICROS);
//    return std::static_pointer_cast<messaging::ValueTuple>(value);
//}

std::shared_ptr<messaging::StringBody> SimCrypto::rsa_sign_blinded(const int caller_id,
        const std::shared_ptr<messaging::StringBody>& blinded_message) {
    if(caller_id > -1) meter_network_clients.at(caller_id).get().delay_client(RSA_SIGN_TIME_MICROS);
    return blinded_message;
}

void SimCrypto::rsa_unblind_signature(const int caller_id,
        const std::shared_ptr<std::string>& blinded_signature, util::SignatureArray& signature) {
    if(caller_id > -1) meter_network_clients.at(caller_id).get().delay_client(RSA_DECRYPT_TIME_MICROS);
    signature.fill(0);
}

void SimCrypto::rsa_sign(const int caller_id, const messaging::SignedValue& value, util::SignatureArray& signature) {
    if(caller_id > -1) meter_network_clients.at(caller_id).get().delay_client(RSA_SIGN_TIME_MICROS);
    signature.fill(0);
}

bool SimCrypto::rsa_verify(const int caller_id, const messaging::SignedValue& value, const util::SignatureArray& signature,
        const int signer_meter_id) {
    if(caller_id > -1) meter_network_clients.at(caller_id).get().delay_client(RSA_VERIFY_TIME_MICROS);
    return true;
}

} /* namespace simulation */
} /* namespace psm */

