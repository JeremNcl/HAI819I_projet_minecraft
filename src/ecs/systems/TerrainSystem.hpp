#pragma once

#include "../registry.hpp"
#include "../components/chunk.hpp"
#include "../components/transform.hpp"
#include "../components/mesh.hpp"
#include "../../modules/terrain_gen/TerrainGenerator.hpp"

class TerrainSystem {
private:
    TerrainGenerator m_generator;

public:
    TerrainSystem(const TerrainConfig& config) : m_generator(config) {}

    void update(Registry& registry) {
        auto view = registry.view<TransformComponent, MeshComponent>();

        for (EntityID entity : view) {
            if (!registry.hasComponent<ChunkComponent>(entity)) {
                
                const auto& transform = registry.getComponent<TransformComponent>(entity);
                
                int chunkX = static_cast<int>(transform.position.x / 16);
                int chunkZ = static_cast<int>(transform.position.z / 16);

                std::vector<BlockType> blocks = m_generator.GenerateChunk(chunkX, chunkZ);
                
                ChunkComponent newChunk(glm::ivec3(chunkX, 0, chunkZ));
                
                for (int y = 0; y < 256; ++y) {
                    for (int z = 0; z < 16; ++z) {
                        for (int x = 0; x < 16; ++x) {
                            int idx = m_generator.GetIndex(x, y, z);
                            
                            VoxelType t = VoxelType::AIR;
                            if (blocks[idx] == BlockType::STONE) t = VoxelType::STONE;
                            else if (blocks[idx] == BlockType::DIRT) t = VoxelType::DIRT;
                            else if (blocks[idx] == BlockType::GRASS) t = VoxelType::GRASS;
                            
                            newChunk.setVoxel(x, y, z, t);
                        }
                    }
                }
                
                registry.addComponent(entity, newChunk);
            }
        }
    }
};