/*
 * UtilityClient.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <memory>

#include "messaging/Message.h"

namespace pddm {

/**
 * The client that runs at the utility to handle sending out query requests and
 * receiving responses.
 */
class UtilityClient {
    public:
        virtual ~UtilityClient() = 0;
        /** Handles receiving an AggregationMessage from a meter, which should contain a query result. */
        virtual void receive_message(const std::shared_ptr<messaging::AggregationMessage>& messages);

        virtual void receive_message(const std::shared_ptr<messaging::SignatureRequest>& message);

};

} /* namespace psm */

