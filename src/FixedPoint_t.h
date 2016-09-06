/**
 * @file FixedPoint_t.h
 * Defines an alias for util::FixedPoint with template parameters configured.
 * This had to be separated out from Configuration.h because it needs to be
 * defined in contexts where the rest of Configuration.h can't be included.
 * @date Aug 16, 2016
 * @author edward
 */

#pragma once

#include "util/FixedPoint.h"

namespace pddm {
using FixedPoint_t = util::FixedPoint<int64_t, 16>;
}
