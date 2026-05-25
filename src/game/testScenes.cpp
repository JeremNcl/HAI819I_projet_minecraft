#include "testScenes.hpp"
#include "ecs/components/chunk.hpp"
#include "ecs/components/mesh.hpp"
#include "ecs/components/transform.hpp"
#include "modules/terrain_gen/TerrainGenerator.hpp"
#include <glm/glm.hpp>

namespace TestScenes {

void createSimpleChunk(Registry& registry) {
    printf("=== TEST SCENE: Simple Chunk ===\n");
    printf("Creating a single chunk parent with 16 SubChunks...\n");

    const glm::vec3 testBiomeColor(0.48f, 0.74f, 0.42f);
    
    EntityID parentChunkEntity = registry.createEntity();
    ChunkComponent chunkManager(glm::ivec2(0, 0));
    
    for (int subY = 0; subY < 16; ++subY) {
        EntityID subChunkEntity = registry.createEntity();
        SubChunkComponent subChunkData(glm::ivec3(0, subY, 0));
        subChunkData.biomeColors.fill(testBiomeColor);
        
        if (subY == 0) {
            for (int y = 0; y < 16; ++y) {
                for (int z = 0; z < 16; ++z) {
                    for (int x = 0; x < 16; ++x) {
                        VoxelType type = VoxelType::AIR;
                        if (y < 10) type = VoxelType::STONE;
                        else if (y < 14) type = VoxelType::DIRT;
                        else if (y < 16) type = VoxelType::GRASS;
                        subChunkData.setVoxel(x, y, z, type);
                    }
                }
            }
        }
        
        subChunkData.meshDirty = true;
        registry.addComponent(subChunkEntity, subChunkData);
        registry.addComponent(subChunkEntity, MeshComponent());
        
        registry.addComponent(subChunkEntity, TransformComponent(glm::vec3(0.0f, 0.0f, 0.0f)));
        
        chunkManager.subChunks[subY] = subChunkEntity;
    }
    
    chunkManager.isFullyGenerated = true;
    registry.addComponent(parentChunkEntity, chunkManager);
}

void createGeneratedChunk(Registry& registry) {
    printf("=== TEST SCENE: Single Procedural Chunk ===\n");
    TerrainConfig config;
    TerrainGenerator generator(config);
    const glm::vec3 testBiomeColor(0.48f, 0.74f, 0.42f);

    int chunksGenerated = 0;
    int radius = 5;
    auto subChunksData = generator.GenerateChunk(0, 0);

    EntityID parentChunkEntity = registry.createEntity();
    ChunkComponent chunkManager(glm::ivec2(0, 0));

    for (int subY = 0; subY < 16; ++subY) {
        EntityID subChunkEntity = registry.createEntity();
        SubChunkComponent subChunk(glm::ivec3(0, subY, 0));
        subChunk.biomeColors.fill(testBiomeColor);

        for (int y = 0; y < 16; ++y) {
            for (int z = 0; z < 16; ++z) {
                for (int x = 0; x < 16; ++x) {
                    int localIndex = x + (z * 16) + (y * 16 * 16);
                    BlockType generatedBlock = subChunksData[subY][localIndex];

                    VoxelType t = VoxelType::AIR;
                    if (generatedBlock == BlockType::STONE) t = VoxelType::STONE;
                    else if (generatedBlock == BlockType::DIRT) t = VoxelType::DIRT;
                    else if (generatedBlock == BlockType::GRASS) t = VoxelType::GRASS;

                    subChunk.setVoxel(x, y, z, t);
                }
            }
        }

        subChunk.meshDirty = true;
        registry.addComponent(subChunkEntity, subChunk);
        registry.addComponent(subChunkEntity, MeshComponent());
        registry.addComponent(subChunkEntity, TransformComponent(glm::vec3(0.0f, 0.0f, 0.0f)));

        chunkManager.subChunks[subY] = subChunkEntity;
    }

    chunkManager.isFullyGenerated = true;
    registry.addComponent(parentChunkEntity, chunkManager);
    printf("✓ Generated 1 procedural chunk at (0, 0)\n");
}

void createInfiniteTerrainScene(Registry& registry) {
    printf("=== TEST SCENE: Infinite Terrain (TerrainSystem + PathFinding) ===\n");
    printf("Chunks will be generated on-demand by TerrainSystem.\n");
}

}  // namespace TestScenes
