#pragma once

#include <cstdint>
#include <limits>

using EntityID = uint32_t;

constexpr EntityID NULL_ENTITY = std::numeric_limits<EntityID>::max();
