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
    if (g_initialized) return;

    // Par défaut, tout pointe vers 0 (la texture d'erreur)
    constexpr int kFallbackSlice = 0;
    g_definitions.fill(makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice));

    // Mapping de la géométrie du monde
    g_definitions[static_cast<size_t>(VoxelType::AIR)]    = makeDefinition(0, 0, 0); // Non rendu
    g_definitions[static_cast<size_t>(VoxelType::STONE)]  = makeDefinition(1, 1, 1);
    g_definitions[static_cast<size_t>(VoxelType::DIRT)]   = makeDefinition(2, 2, 2);
    g_definitions[static_cast<size_t>(VoxelType::GRASS)]  = makeDefinition(3, 2, 4); // Haut=3, Bas=2, Côtés=4
    
    // Nouveaux Blocs configurés
    g_definitions[static_cast<size_t>(VoxelType::SAND)]   = makeDefinition(5, 5, 5);
    g_definitions[static_cast<size_t>(VoxelType::WOOD)]   = makeDefinition(6, 6, 7); // Section tronc=6, Écorce=7
    g_definitions[static_cast<size_t>(VoxelType::LEAVES)] = makeDefinition(8, 8, 8);

    // Les blocs ci-dessous afficheront la grille d'erreur en attendant leurs assets PBR
    g_definitions[static_cast<size_t>(VoxelType::BEDROCK)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::COAL)]    = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::IRON)]    = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::GOLD)]    = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::DIAMOND)] = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::LAVA)]    = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);
    g_definitions[static_cast<size_t>(VoxelType::WATER)]   = makeDefinition(kFallbackSlice, kFallbackSlice, kFallbackSlice);

    g_initialized = true;
}

const BlockRenderDefinition& BlockDefinitionRegistry::get(VoxelType type) {
    if (!g_initialized) initialize();
    size_t index = static_cast<size_t>(type);
    if (index >= g_definitions.size()) return getDefaultDefinition();
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