#pragma once

#include "../registry.hpp"
#include "../components/chunk.hpp"
#include "../components/transform.hpp"
#include "../components/mesh.hpp"
#include "../components/camera.hpp"
#include "../../modules/ChunkWorker.hpp"
#include <map>
#include <utility>
#include <cmath>

class TerrainSystem {
private:
    ChunkWorker m_worker;
    
    std::map<std::pair<int, int>, EntityID> activeChunks;
    
    int renderDistance = 8;
    int unloadDistance = 12;

    void destroyChunkRecursive(Registry& registry, EntityID parentEntity) {
        if (registry.hasComponent<ChunkComponent>(parentEntity)) {
            auto& chunkComp = registry.getComponent<ChunkComponent>(parentEntity);
            for (EntityID subChunkID : chunkComp.subChunks) {
                if (subChunkID != 0) {
                    
                    if (registry.hasComponent<MeshComponent>(subChunkID)) {
                        auto& mesh = registry.getComponent<MeshComponent>(subChunkID);
                        mesh.cleanup();
                    }

                    registry.destroyEntity(subChunkID);
                }
            }
        }
        registry.destroyEntity(parentEntity);
    }

public:
    TerrainSystem(const TerrainConfig& config) : m_worker(config) {}

    int getLoadedChunksCount() const {
        int count = 0;
        for (const auto& [coords, entity] : activeChunks) {
            if (entity != 0) {
                count++;
            }
        }
        return count;
    }

    void update(Registry& registry) {
        glm::vec3 playerPos(0.0f);
        bool foundPlayer = false;
        
        auto camView = registry.view<CameraComponent, TransformComponent>();
        for (EntityID e : camView) {
            if (registry.getComponent<CameraComponent>(e).isActive) {
                playerPos = registry.getComponent<TransformComponent>(e).position;
                foundPlayer = true;
                break;
            }
        }

        if (!foundPlayer) return;

        int playerChunkX = static_cast<int>(std::floor(playerPos.x / 16.0f));
        int playerChunkZ = static_cast<int>(std::floor(playerPos.z / 16.0f));

        for (int x = playerChunkX - renderDistance; x <= playerChunkX + renderDistance; ++x) {
            for (int z = playerChunkZ - renderDistance; z <= playerChunkZ + renderDistance; ++z) {
                std::pair<int, int> coords = {x, z};

                if (activeChunks.find(coords) == activeChunks.end()) {
                    EntityID parentEntity = registry.createEntity();
                    activeChunks[coords] = parentEntity;
                    m_worker.requestChunk(x, z);
                }
            }
        }

        for (auto it = activeChunks.begin(); it != activeChunks.end(); ) {
            int dx = std::abs(it->first.first - playerChunkX);
            int dz = std::abs(it->first.second - playerChunkZ);

            if (dx > unloadDistance || dz > unloadDistance) {
                if (it->second != 0) {
                    destroyChunkRecursive(registry, it->second);
                }
                it = activeChunks.erase(it);
            } else {
                ++it;
            }
        }

        ChunkTask result;
        while (m_worker.popResult(result)) {
            std::pair<int, int> coords = {result.x, result.z};
            
            if (activeChunks.find(coords) != activeChunks.end()) {
                EntityID parentEntity = activeChunks[coords];
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
                }

                chunkManager.isFullyGenerated = true;
                registry.addComponent(parentEntity, chunkManager);

                std::pair<int, int> neighbors[4] = {
                    {result.x + 1, result.z}, {result.x - 1, result.z},
                    {result.x, result.z + 1}, {result.x, result.z - 1}
                };
                for (const auto& n : neighbors) {
                    if (activeChunks.find(n) != activeChunks.end()) {
                        EntityID neighborParent = activeChunks[n];
                        if (neighborParent != 0 && registry.hasComponent<ChunkComponent>(neighborParent)) {
                            auto& neighborChunk = registry.getComponent<ChunkComponent>(neighborParent);
                            for (EntityID subID : neighborChunk.subChunks) {
                                if (subID != 0 && registry.hasComponent<SubChunkComponent>(subID)) {
                                    registry.getComponent<SubChunkComponent>(subID).meshDirty = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
};