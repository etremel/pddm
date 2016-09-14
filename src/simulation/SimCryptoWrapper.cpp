/*
 * SimCrypto.cpp
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#include "SimCryptoWrapper.h"

#include "../MeterClient.h"
#include "../UtilityClient.h"

namespace pddm {
namespace simulation {


std::function<SimCryptoWrapper (MeterClient&)> crypto_library_builder(SimCrypto& crypto_instance) {
    return [&crypto_instance](MeterClient& client) {
        //Ugh, if it wasn't for the need to do this, I wouldn't have to expose MeterClient's network client to everyone
        crypto_instance.add_meter(client.meter_id, client.get_network_client());
        return SimCryptoWrapper(crypto_instance, client.meter_id);
    };
}

std::function<SimCryptoWrapper (UtilityClient&)> crypto_library_builder_utility(SimCrypto& crypto_instance) {
     return [&crypto_instance](UtilityClient& client) {
         return SimCryptoWrapper(crypto_instance, -1);
     };
 }


} /* namespace simulation */
} /* namespace pddm */

