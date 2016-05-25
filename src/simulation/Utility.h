/*
 * Utility.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

namespace pddm {
namespace simulation {

/**
 * Simulation of the utility that interacts with meters in the simulation.
 */
class Utility: public UtilityClient {
    public:
        Utility();
        virtual ~Utility();
        void receive_message(const std::shared_ptr<messaging::Message>& messages) override;
};

} /* namespace simulation */
} /* namespace psm */

