#include "BlockDefinitionRegistry.hpp"

#include <array>

namespace {
constexpr size_t kVoxelTypeCount = static_cast<size_t>(VoxelType::WATER) + 1;

std::array<BlockRenderDefinition, kVoxelTypeCount> g_definitions{};
bool g_initialized = false;

BlockRenderDefinition makeDefinition(int topSlice, int bottomSlice, int sideSlice) {
    BlockRenderDefinition definition;
    definition.topSlice = topSlice;
    definition.bottomSlice = bottomSlice;
    definition.sideSlice = sideSlice;
    return definition;
}
} // namespace

void BlockDefinitionRegistry::initialize() {
    if (g_initialized) {
        return;
    }

    g_definitions.fill(makeDefinition(0, 0, 0));

    constexpr int kFallbackSlice = 4;

    g_definitions[static_cast<size_t>(VoxelType::AIR)] = makeDefinition(0, 0, 0);
    g_definitions[static_cast<size_t>(VoxelType::STONE)] = makeDefinition(0, 0, 0);
    g_definitions[static_cast<size_t>(VoxelType::DIRT)] = makeDefinition(1, 1, 1);
    g_definitions[static_cast<size_t>(VoxelType::GRASS)] = makeDefinition(2, 1, 3);
    g_definitions[static_cast<size_t>(VoxelType::WOOD)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::LEAVES)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::BEDROCK)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::COAL)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::IRON)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::GOLD)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::DIAMOND)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::LAVA)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::SAND)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::WATER)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);

    g_initialized = true;
}

const BlockRenderDefinition& BlockDefinitionRegistry::get(VoxelType type) {
    if (!g_initialized) {
        initialize();
    }

    size_t index = static_cast<size_t>(type);
    if (index >= g_definitions.size()) {
        return getDefaultDefinition();
    }
    return g_definitions[index];
}

int BlockDefinitionRegistry::getTextureSliceIndex(VoxelType type, int axis, bool isPositive) {
    const BlockRenderDefinition& definition = get(type);

    if (axis == 1) {
        return isPositive ? definition.topSlice : definition.bottomSlice;
    }

    return definition.sideSlice;
}

const BlockRenderDefinition& BlockDefinitionRegistry::getDefaultDefinition() {
    static const BlockRenderDefinition defaultDefinition{};
    return defaultDefinition;
}