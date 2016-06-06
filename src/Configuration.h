/*
 * Configuration.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

//Include the implementations here, so the client doesn't have to know which ones to include
#include "simulation/SimNetworkClient.h"
#include "simulation/SimUtilityNetworkClient.h"
#include "util/FixedPoint.h"
#include "CtProtocolState.h"
#include "simulation/Meter.h"
#include "simulation/SimCryptoWrapper.h"
#include "simulation/SimTimerManager.h"

namespace pddm {

//Forward declaration because this header must be included in MeterClient.h
class MeterClient;

using ProtocolState_t = CtProtocolState;
using FixedPoint_t = util::FixedPoint<long long, 16>;
using Meter_t = simulation::Meter;
using NetworkClient_t = simulation::SimNetworkClient;
using UtilityNetworkClient_t = simulation::SimUtilityNetworkClient;
using TimerManager_t = simulation::SimTimerManager;
using CryptoLibrary_t = simulation::SimCryptoWrapper;

using NetworkClientBuilderFunc = std::function<NetworkClient_t (MeterClient&)>;
using MeterBuilderFunc = std::function<Meter_t (MeterClient&)>;
using CryptoLibraryBuilderFunc = std::function<CryptoLibrary_t (MeterClient&)>;
using TimerManagerBuilderFunc = std::function<TimerManager_t (MeterClient&)>;

}



