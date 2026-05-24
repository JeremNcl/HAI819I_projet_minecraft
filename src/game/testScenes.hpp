#pragma once

#include "../ecs/registry.hpp"
#include "../ecs/components/world.hpp"

namespace TestScenes {
    static WorldMapComponent& getOrCreateWorldMap(Registry& registry);
    void createSimpleChunk(Registry& registry);
    void createTerrainChunk(Registry& registry);
    void createDynamicTerrainScene(Registry& registry);
}
