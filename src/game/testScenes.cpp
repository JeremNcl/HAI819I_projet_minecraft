#include "testScenes.hpp"
#include "../ecs/components/chunk.hpp"
#include "../ecs/components/mesh.hpp"
#include "../ecs/components/transform.hpp"
#include "../modules/terrain_gen/TerrainGenerator.hpp"
#include <glm/glm.hpp>

namespace TestScenes {

static WorldMapComponent& getOrCreateWorldMap(Registry& registry) {
    auto view = registry.view<WorldMapComponent>();
    for (EntityID entity : view) {
        return registry.getComponent<WorldMapComponent>(entity);
    }
    
    EntityID worldEntity = registry.createEntity();
    registry.addComponent(worldEntity, WorldMapComponent());
    return registry.getComponent<WorldMapComponent>(worldEntity);
}

void createSimpleChunk(Registry& registry) {
    printf("=== TEST SCENE: Simple Chunk ===\n");
    printf("Creating a single chunk parent with 16 SubChunks...\n");

    WorldMapComponent& worldMap = getOrCreateWorldMap(registry);

    EntityID parentChunkEntity = registry.createEntity();
    ChunkComponent chunkManager(glm::ivec2(0, 0));
    
    for (int subY = 0; subY < 16; ++subY) {
        EntityID subChunkEntity = registry.createEntity();
        SubChunkComponent subChunkData(glm::ivec3(0, subY, 0));
        
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

        worldMap.subChunkEntities[subChunkData.subChunkPosition] = subChunkEntity;
    }
    
    chunkManager.isFullyGenerated = true;
    registry.addComponent(parentChunkEntity, chunkManager);
}

void createTerrainChunk(Registry& registry) {
    printf("=== TEST SCENE: Procedural Terrain ===\n");
    TerrainConfig config;
    TerrainGenerator generator(config);

    WorldMapComponent& worldMap = getOrCreateWorldMap(registry);

    int chunksGenerated = 0;
    int radius = 5;

    for (int chunkX = -radius; chunkX <= radius; ++chunkX) {
        for (int chunkZ = -radius; chunkZ <= radius; ++chunkZ) {
            
            auto subChunksData = generator.GenerateChunk(chunkX, chunkZ);
            
            EntityID parentChunkEntity = registry.createEntity();
            ChunkComponent chunkManager(glm::ivec2(chunkX, chunkZ));

            for (int subY = 0; subY < 16; ++subY) {
                EntityID subChunkEntity = registry.createEntity();
                SubChunkComponent subChunk(glm::ivec3(chunkX, subY, chunkZ));
                
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
                worldMap.subChunkEntities[subChunk.subChunkPosition] = subChunkEntity;
            }
            
            chunkManager.isFullyGenerated = true;
            registry.addComponent(parentChunkEntity, chunkManager);
            chunksGenerated++;
        }
    }
    printf("✓ Generated %d procedural terrain chunks\n", chunksGenerated);
}

void createDynamicTerrainScene(Registry& registry) {
    printf("=== TEST SCENE: Dynamic Terrain (TerrainSystem + PathFinding) ===\n");
    printf("This scene will use TerrainSystem for dynamic chunk generation\n");
    printf("and PathFindingSystem for AI pathfinding across multiple chunks.\n");
    printf("Note: Chunks will be generated on-demand by TerrainSystem\n");
    
    // Empty scene - chunks will be generated dynamically by TerrainSystem
    // during the ECS update loop

    getOrCreateWorldMap(registry);
}

}  // namespace TestScenes
