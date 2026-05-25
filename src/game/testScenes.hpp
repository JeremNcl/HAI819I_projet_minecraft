#pragma once

#include "../ecs/registry.hpp"
#include "../ecs/components/world.hpp"

namespace TestScenes {
    static WorldMapComponent& getOrCreateWorldMap(Registry& registry);
    void createSimpleChunk(Registry& registry);
    void createGeneratedChunk(Registry& registry);
    void createInfiniteTerrainScene(Registry& registry);
}
