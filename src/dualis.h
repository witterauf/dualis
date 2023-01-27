#pragma once

// TODO: configure defines using compiler macros?
#define _DUALIS_UNALIGNED_MEM_ACCESS

#include "concepts.h"
#include "utilities.h"
#include "containers.h"
#include "packing.h"
#include "streams.h"

#include <bit>

namespace dualis {
#include "containers_impl.h"
#include "utilities_impl.h"
}