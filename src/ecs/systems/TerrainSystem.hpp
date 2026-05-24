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

    void update(Registry& registry, WorldMapComponent& worldMap) {
        auto view = registry.view<TransformComponent, MeshComponent>();

        for (EntityID entity : view) {
            if (!registry.hasComponent<ChunkComponent>(entity) && !registry.hasComponent<SubChunkComponent>(entity)) {
                
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
                EntityID parentEntity = pendingRequests[coords];
                ChunkComponent chunkManager(glm::ivec2(result.x, result.z));
                
                for (int subY = 0; subY < 16; ++subY){
                    EntityID subChunkEntity = registry.createEntity();
                    SubChunkComponent subChunk(glm::ivec3(result.x, subY, result.z));

                    for (int y = 0; y < 16; ++y) {
                        for (int z = 0; z < 16; ++z) {
                            for (int x = 0; x < 16; ++x) {
                                int localIdx = x + (z * 16) + (y * 16 * 16); 
                                
                                BlockType genBlock = result.data[subY][localIdx];

                                VoxelType t = VoxelType::AIR;
                                if (genBlock == BlockType::STONE) t = VoxelType::STONE;
                                else if (genBlock == BlockType::DIRT) t = VoxelType::DIRT;
                                else if (genBlock == BlockType::GRASS) t = VoxelType::GRASS;
                                
                                subChunk.setVoxel(x, y, z, t);
                            }
                        }
                    }
                    subChunk.meshDirty = true;
                    registry.addComponent(subChunkEntity, subChunk);
                    registry.addComponent(subChunkEntity, MeshComponent());
                    registry.addComponent(subChunkEntity, TransformComponent(glm::vec3(0.0f,0.0f,0.0f)));
                    chunkManager.subChunks[subY] = subChunkEntity;

                    worldMap.subChunkEntities[subChunk.subChunkPosition] = subChunkEntity;
                }

                chunkManager.isFullyGenerated = true;
                registry.addComponent(parentEntity, chunkManager);
                pendingRequests.erase(coords);
            }
        }
    }
};