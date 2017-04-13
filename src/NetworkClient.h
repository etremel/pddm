/*
 * Network.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <list>

#include "messaging/OverlayTransportMessage.h"
#include "messaging/AggregationMessage.h"
#include "messaging/PingMessage.h"
#include "messaging/SignatureRequest.h"

namespace pddm {


/**
 * Defines the interface for the network client used by a MeterClient to
 * interact with the network.
 */
class NetworkClient {
    public:
        virtual ~NetworkClient() = 0;
        /**
         * Sends a stream of overlay messages over the network to another meter,
         * identified by its ID. Messages will be sent in the order they appear
         * in the list.
         * @param messages The messages to send.
         * @param recipient_id The ID of the recipient.
         * @return true if the send was successful, false if a connection could not be made.
         */
        virtual bool send(const std::list<std::shared_ptr<messaging::OverlayTransportMessage>>& messages, const int recipient_id) = 0;
        /**
         * Sends an AggregationMessage over the network to another meter (or the
         * utility), identified by its ID.
         * @param message The message to send
         * @param recipient_id The ID of the recipient
         * @return true if the send was successful, false if a connection could not be made.
         */
        virtual bool send(const std::shared_ptr<messaging::AggregationMessage>& message, const int recipient_id) = 0;
        /**
         * Sends a PingMessage over the network to another meter
         * @param message The message to send
         * @param recipient_id The ID of the recipient
         * @return true if the send was successful, false if a connection could not be made.
         */
        virtual bool send(const std::shared_ptr<messaging::PingMessage>& message, const int recipient_id) = 0;
        /** Sends a signature request message to the utility. */
        virtual bool send(const std::shared_ptr<messaging::SignatureRequest>& message) = 0;

        /**
         * Continuously polls for incoming messages to this meter, calling the
         * appropriate "handler" function in MeterClient each time a message is
         * received. This function will never return, so each meter should call
         * it once, in its own thread.
         */
        virtual void monitor_incoming_messages() = 0;
};

inline NetworkClient::~NetworkClient() { }

}


