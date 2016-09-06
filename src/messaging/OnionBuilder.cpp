/**
 * @file OnionBuilder.cpp
 *
 * @date Aug 17, 2016
 * @author edward
 */

#include <memory>
#include <list>

#include "OnionBuilder.h"
#include "../Configuration.h"
#include "../ConfigurationIncludes.h"
#include "MessageBody.h"
#include "OverlayMessage.h"

namespace pddm {
namespace messaging {

std::shared_ptr<OverlayMessage> build_encrypted_onion(const std::list<int>& path, const std::shared_ptr<MessageBody>& payload,
        const int query_num, CryptoLibrary_t& crypto_library) {
    //Start with the last layer of the onion, which actually contains the payload
    auto current_layer = crypto_library.rsa_encrypt(std::make_shared<OverlayMessage>(
            query_num, path.back(), payload), path.back());
    //Build the onion from the end of the list to the beginning
    for(auto path_iter = path.rbegin(); path_iter != path.rend(); ++path_iter) {
        if(path_iter == path.rbegin())
            continue;
        //Make the previous layer the payload of a new message, whose destination is the next hop
        auto next_layer = crypto_library.rsa_encrypt(std::make_shared<OverlayMessage>(
                query_num, *path_iter, current_layer), *path_iter);
        current_layer = next_layer;
    }
    return current_layer;
}

} /* namespace messaging */
} /* namespace pddm */

