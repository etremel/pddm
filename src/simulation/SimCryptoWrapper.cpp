/*
 * SimCrypto.cpp
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#include "SimCryptoWrapper.h"

#include <functional>

#include "../Configuration.h"

namespace pddm {
namespace simulation {


CryptoLibraryBuilderFunc crypto_library_builder(SimCrypto& crypto_instance) {
    return [&crypto_instance](MeterClient& client) {
        crypto_instance.add_meter(client.meter_id, client.network);
        return SimCryptoWrapper(crypto_instance, client.meter_id);
    };
}

 std::function<CryptoLibrary_t (UtilityClient&)> crypto_library_builder_utility(SimCrypto& crypto_instance) {
     return [&crypto_instance](UtilityClient& client) {
         return SimCryptoWrapper(crypto_instance, -1);
     };
 }


} /* namespace simulation */
} /* namespace psm */

