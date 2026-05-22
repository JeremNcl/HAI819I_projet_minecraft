#pragma once

#include "../registry.hpp"
#include "../components/chunk.hpp"
#include "../components/transform.hpp"
#include "../components/mesh.hpp"
#include "../../modules/ChunkWorker.hpp"
#include <map>

class TerrainSystem {
private:
    ChunkWorker m_worker;
    std::map<std::pair<int, int>, EntityID> pendingRequests;

public:
    TerrainSystem(const TerrainConfig& config) : m_worker(config) {}

    void update(Registry& registry) {
        auto view = registry.view<TransformComponent, MeshComponent>();

        for (EntityID entity : view) {
            if (!registry.hasComponent<ChunkComponent>(entity)) {
                
                const auto& transform = registry.getComponent<TransformComponent>(entity);
                int chunkX = static_cast<int>(transform.position.x / 16);
                int chunkZ = static_cast<int>(transform.position.z / 16);
                std::pair<int, int> coords = {chunkX, chunkZ};

                if (pendingRequests.find(coords) == pendingRequests.end()) {
                    pendingRequests[coords] = entity;
                    m_worker.requestChunk(chunkX, chunkZ);
                }
            }
        }

        ChunkTask result;
        while (m_worker.popResult(result)) {
            std::pair<int, int> coords = {result.x, result.z};
            
            if (pendingRequests.find(coords) != pendingRequests.end()) {
                EntityID entity = pendingRequests[coords];
                ChunkComponent newChunk(glm::ivec3(result.x, 0, result.z));
                
                for (int y = 0; y < 256; ++y) {
                    for (int z = 0; z < 16; ++z) {
                        for (int x = 0; x < 16; ++x) {
                            int idx = x + (z * 16) + (y * 16 * 16); 
                            
                            VoxelType t = VoxelType::AIR;
                            if (result.data[idx] == BlockType::STONE) t = VoxelType::STONE;
                            else if (result.data[idx] == BlockType::DIRT) t = VoxelType::DIRT;
                            else if (result.data[idx] == BlockType::GRASS) t = VoxelType::GRASS;
                            
                            newChunk.setVoxel(x, y, z, t);
                        }
                    }
                }
                
                newChunk.meshDirty = true;
                registry.addComponent(entity, newChunk);
                
                pendingRequests.erase(coords);
            }
        }
    }
};