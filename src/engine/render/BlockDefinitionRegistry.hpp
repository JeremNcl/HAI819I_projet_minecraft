#pragma once

#include "ecs/components/chunk.hpp"

struct BlockRenderDefinition {
    int topSlice = 0;
    int bottomSlice = 0;
    int sideSlice = 0;
};

class BlockDefinitionRegistry {
public:
    static void initialize();
    static const BlockRenderDefinition& get(VoxelType type);
    static int getTextureSliceIndex(VoxelType type, int axis, bool isPositive);

private:
    static const BlockRenderDefinition& getDefaultDefinition();
};