/*
 * SimCrypto.cpp
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#include "SimCrypto.h"


namespace pddm {
namespace simulation {

using util::RSA_SIGNATURE_SIZE;

void SimCrypto::add_meter(const int meter_id, const SimNetworkClient& meter_network_client) {
    meter_network_clients[meter_id] = std::ref(meter_network_client);
}


std::shared_ptr<messaging::OverlayMessage> SimCrypto::rsa_encrypt_message(const int caller_id,
        const std::shared_ptr<messaging::OverlayMessage>& message) {
    //The utility has id -1, and doesn't get delayed because it's a datacenter
    if(caller_id > -1) meter_network_clients[caller_id].get().delay_client(RSA_ENCRYPT_TIME_MICROS);
    return message;
}

std::shared_ptr<messaging::OverlayMessage> SimCrypto::rsa_decrypt_message(const int caller_id,
        const std::shared_ptr<messaging::OverlayMessage>& message) {
    if(caller_id > -1) meter_network_clients[caller_id].get().delay_client(RSA_DECRYPT_TIME_MICROS);
    return message;
}

void SimCrypto::rsa_sign_value(const int caller_id, const messaging::ValueTuple& value,
        util::SignatureArray& signature) {
    if(caller_id > -1) meter_network_clients[caller_id].get().delay_client(RSA_SIGN_TIME_MICROS);
}

bool SimCrypto::rsa_verify_value(const int caller_id, const messaging::ValueTuple& value,
        const util::SignatureArray& signature) {
    if(caller_id > -1) meter_network_clients[caller_id].get().delay_client(RSA_VERIFY_TIME_MICROS);
    return true;
}

std::shared_ptr<messaging::MessageBody> SimCrypto::rsa_encrypt_value(const int caller_id,
        const std::shared_ptr<messaging::ValueContribution>& value) {
    if(caller_id > -1) meter_network_clients[caller_id].get().delay_client(RSA_ENCRYPT_TIME_MICROS);
    return value;
}

std::shared_ptr<messaging::ValueContribution> SimCrypto::rsa_decrypt_value(const int caller_id,
        const std::shared_ptr<messaging::MessageBody>& value) {
    if(caller_id > -1) meter_network_clients[caller_id].get().delay_client(RSA_DECRYPT_TIME_MICROS);
    //In the simulation, this will be exactly the same ValueContribution that was earlier passed to encrypt_value
    return std::static_pointer_cast<messaging::ValueContribution>(value);
}

} /* namespace simulation */
} /* namespace psm */


