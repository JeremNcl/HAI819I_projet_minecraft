#pragma once

#include "../../ecs/registry.hpp"
#include "../components/timeComponent.hpp"

class TimeSystem {
public:
    void update(Registry& registry, float deltaTime);
};
