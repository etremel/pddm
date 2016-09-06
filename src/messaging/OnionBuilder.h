/**
 * @file OnionBuilder.h
 *
 * @date Aug 17, 2016
 * @author edward
 */

#pragma once

#include <memory>
#include <list>

#include "../Configuration.h"
#include "OverlayMessage.h"

namespace pddm {
namespace messaging {

std::shared_ptr<OverlayMessage> build_encrypted_onion(const std::list<int>& path, const std::shared_ptr<MessageBody>& payload,
        const int query_num, CryptoLibrary_t& crypto_library);

} /* namespace messaging */
} /* namespace pddm */


