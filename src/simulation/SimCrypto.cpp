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


std::shared_ptr<messaging::OverlayMessage> SimCrypto::rsa_encrypt_message(const int caller_id, const std::shared_ptr<messaging::OverlayMessage>& message) {
    meter_network_clients[caller_id].get().delay_client(RSA_ENCRYPT_TIME_MICROS);
    return message;
}

std::shared_ptr<messaging::OverlayMessage> SimCrypto::rsa_decrypt_message(const int caller_id, const std::shared_ptr<messaging::OverlayMessage>& message) {
    meter_network_clients[caller_id].get().delay_client(RSA_DECRYPT_TIME_MICROS);
    return message;
}

void SimCrypto::rsa_sign_value(const int caller_id, const messaging::ValueTuple& value, uint8_t (&signature)[RSA_SIGNATURE_SIZE]) {
    meter_network_clients[caller_id].get().delay_client(RSA_SIGN_TIME_MICROS);
}

bool SimCrypto::rsa_verify_value(const int caller_id, const messaging::ValueTuple& value, const uint8_t (&signature)[RSA_SIGNATURE_SIZE]) {
    meter_network_clients[caller_id].get().delay_client(RSA_VERIFY_TIME_MICROS);
    return true;
}


} /* namespace simulation */
} /* namespace psm */
