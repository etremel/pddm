/**
 * @file DebugState.cpp
 *
 * @date Sep 10, 2016
 * @author edward
 */



#include "DebugState.h"

namespace pddm {
namespace util {

DebugState& debug_state() {
    static DebugState instance;
    return instance;
}

}  // namespace util
}
