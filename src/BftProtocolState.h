/*
 * BftProtocolState.h
 *
 *  Created on: May 24, 2016
 *      Author: edward
 */

#pragma once

#include "ProtocolState.h"
#include "messaging/SignatureResponse.h"

namespace pddm {

class BftProtocolState: public ProtocolState<BftProtocolState> {
    public:
        BftProtocolState();
        virtual ~BftProtocolState();
        void handle_signature_response(const std::shared_ptr<messaging::SignatureResponse>& message);
        void start_query_impl(const messaging::QueryRequest& query_request, const std::vector<FixedPoint_t>& contributed_data);
        void handle_overlay_message_impl(const std::shared_ptr<messaging::OverlayTransportMessage>& message);

};

} /* namespace psm */

