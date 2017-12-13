/**
 * @file ConfigurationIncludes.h
 * This file contains include statements for all of the types defined in
 * Configuration.h, and should be the last include statement for any file that
 * needs to include Configuration.h. If Configuration.h is changed to define
 * different implementations for its types, the include statements here should
 * be changed accordingly.
 * @date Aug 9, 2016
 * @author edward
 */

#pragma once

//#include "simulation/SimNetworkClient.h"
#include "networking/TcpNetworkClient.h"
//#include "simulation/SimUtilityNetworkClient.h"
#include "networking/TcpUtilityClient.h"
#include "simulation/Meter.h"
//#include "simulation/SimCryptoWrapper.h"
//#include "util/DummyCrypto.h"
#include "util/CryptoPPLibrary.h"
//#include "simulation/SimTimerManager.h"
#include "util/LinuxTimerManager.h"
//#include "HftProtocolState.h"
#include "CtProtocolState.h"
//#include "BftProtocolState.h"
