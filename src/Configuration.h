/**
 * @file Configuration.h
 * This file configures the program by defining several abstract types to be
 * one of several possible concrete implementations. Specifically, when the
 * smart meter code is run in simulation mode, the definitions here should be
 * changed to refer to "simulation" objects rather than real implementations.
 * @date May 17, 2016
 * @author edward
 */

#pragma once

#include <functional>
#include <cstdint>

namespace pddm {
//Forward declare these types to avoid circular includes of configuration.h
class CtProtocolState;
class BftProtocolState;
class HftProtocolState;
class MeterClient;

namespace simulation {
class Meter;
class SimNetworkClient;
class SimUtilityNetworkClient;
class SimTimerManager;
class SimCryptoWrapper;
} /* namespace simulation */

using ProtocolState_t = CtProtocolState;
using Meter_t = simulation::Meter;
using NetworkClient_t = simulation::SimNetworkClient;
using UtilityNetworkClient_t = simulation::SimUtilityNetworkClient;
using TimerManager_t = simulation::SimTimerManager;
using CryptoLibrary_t = simulation::SimCryptoWrapper;

using NetworkClientBuilderFunc = std::function<NetworkClient_t (MeterClient&)>;
using MeterBuilderFunc = std::function<Meter_t (MeterClient&)>;
using CryptoLibraryBuilderFunc = std::function<CryptoLibrary_t (MeterClient&)>;
using TimerManagerBuilderFunc = std::function<TimerManager_t (MeterClient&)>;

} /* namespace pddm */



