#include "testScenes.hpp"
#include "../ecs/components/chunk.hpp"
#include "../ecs/components/mesh.hpp"
#include "../ecs/components/transform.hpp"
#include "../modules/terrain_gen/TerrainGenerator.hpp"
#include <glm/glm.hpp>

namespace TestScenes {

void createSimpleChunk(Registry& registry) {
    printf("=== TEST SCENE: Simple Chunk ===\n");
    printf("Creating a single chunk with thick Stone, Dirt, and Grass layers...\n");
    
    EntityID testChunkEntity = registry.createEntity();
    
    ChunkComponent chunkData(glm::ivec3(0, 0, 0));
    
    // Fill the chunk with thick layers for easy visual testing
    // Stone: layers 0-9 (10 layers thick)
    // Dirt:  layers 10-13 (4 layers thick)
    // Grass: layers 14-15 (2 layers thick)
    // Air:   layers 16+ (empty space above)
    
    for (int y = 0; y < 16; ++y) {  // CHUNK_SIZE_Y = 256, but test with first 16 layers
        for (int z = 0; z < 16; ++z) {
            for (int x = 0; x < 16; ++x) {
                VoxelType voxelType = VoxelType::AIR;
                
                if (y < 10) {
                    voxelType = VoxelType::STONE;
                } else if (y < 14) {
                    voxelType = VoxelType::DIRT;
                } else if (y < 16) {
                    voxelType = VoxelType::GRASS;
                }
                
                chunkData.setVoxel(x, y, z, voxelType);
            }
        }
    }
    
    chunkData.meshDirty = true;
    
    registry.addComponent(testChunkEntity, chunkData);
    registry.addComponent(testChunkEntity, MeshComponent());
    registry.addComponent(testChunkEntity, TransformComponent(glm::vec3(0.0f, 0.0f, 0.0f)));
    
    printf("✓ Simple chunk created: 16x16x16 cube with Stone(0-9), Dirt(10-13), Grass(14-15)\n");
}

void createTerrainChunk(Registry& registry) {
    printf("=== TEST SCENE: Terrain Generator ===\n");
    printf("Generating procedural terrain chunks...\n");
    
    TerrainConfig config = LoadConfig("config.txt");
    TerrainGenerator generator(config);

    int numChunksX = 5;
    int numChunksZ = 5;
    int chunksGenerated = 0;

    for (int chunkX = 0; chunkX < numChunksX; ++chunkX) {
        for (int chunkZ = 0; chunkZ < numChunksZ; ++chunkZ) {
            EntityID currentChunkEntity = registry.createEntity();
            ChunkComponent chunkData(glm::ivec3(chunkX, 0, chunkZ));
            std::vector<BlockType> proceduralBlocks = generator.GenerateChunk(chunkX, chunkZ);
            
            for (int y = 0; y < TerrainGenerator::CHUNK_HEIGHT; ++y) {
                for (int z = 0; z < TerrainGenerator::CHUNK_DEPTH; ++z) {
                    for (int x = 0; x < TerrainGenerator::CHUNK_WIDTH; ++x) {
                        int index = generator.GetIndex(x, y, z);
                        BlockType myBlock = proceduralBlocks[index];
                        VoxelType theirType = VoxelType::AIR;
                        
                        switch (myBlock) {
                            case BlockType::GRASS:   theirType = VoxelType::GRASS; break;
                            case BlockType::DIRT:    theirType = VoxelType::DIRT; break;
                            case BlockType::STONE:   theirType = VoxelType::STONE; break;
                            default: break;
                        }
                        
                        chunkData.setVoxel(x, y, z, theirType);
                    }
                }
            }

            chunkData.meshDirty = true;
            
            registry.addComponent(currentChunkEntity, chunkData);
            registry.addComponent(currentChunkEntity, MeshComponent());
            
            float worldPosX = chunkX * TerrainGenerator::CHUNK_WIDTH;
            float worldPosZ = chunkZ * TerrainGenerator::CHUNK_DEPTH;
            registry.addComponent(currentChunkEntity, TransformComponent(glm::vec3(worldPosX, 0.0f, worldPosZ)));

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
}

}  // namespace TestScenes
