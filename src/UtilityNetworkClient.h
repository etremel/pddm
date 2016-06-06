/*
 * UtilityNetworkClient.h
 *
 *  Created on: May 31, 2016
 *      Author: edward
 */

#pragma once

#include <memory>

#include "messaging/QueryRequest.h"
#include "messaging/SignatureResponse.h"

namespace pddm {

/**
 * Defines the interface for the network client used by a UtilityClient to
 * interact with the network.
 */
class UtilityNetworkClient {
    public:
        virtual ~UtilityNetworkClient() = 0;
        /**
         * Sends a query request message to the meter with the specified ID.
         * @param message The message to send
         * @param recipient_id The ID of the recipient
         */
        virtual void send(const std::shared_ptr<messaging::QueryRequest>& message, const int recipient_id);
        /**
         * Sends a signature response (blindly signed value) back to a meter.
         * @param message The message to send
         * @param recipient_id The ID of the recipient
         */
        virtual void send(const std::shared_ptr<messaging::SignatureResponse>& message, const int recipient_id);
};

}


