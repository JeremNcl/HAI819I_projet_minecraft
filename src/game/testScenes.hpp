#pragma once

#include "ecs/registry.hpp"

namespace TestScenes {
    void createSimpleChunk(Registry& registry);
    void createTerrainChunk(Registry& registry);
    void createDynamicTerrainScene(Registry& registry);
}
