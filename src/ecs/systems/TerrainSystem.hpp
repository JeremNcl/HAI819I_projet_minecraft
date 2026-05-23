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
#include <queue>
#include <mutex>
#include <set>

class TerrainSystem {
private:
    ChunkWorker m_worker;
    std::set<EntityID> meshingQueue;
    std::mutex queueMutex;
    
    std::map<std::pair<int, int>, EntityID> activeChunks;
    
    int renderDistance = 14;
    int unloadDistance = 18;

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

    void requestMesh(EntityID id) {
        std::lock_guard<std::mutex> lock(queueMutex);
        meshingQueue.insert(id);
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

    bool popMeshingTask(EntityID& outID) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (meshingQueue.empty()) return false;
        outID = *meshingQueue.begin(); 
        meshingQueue.erase(meshingQueue.begin());
        return true;
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
                                
                                switch(genBlock) {
                                    case BlockType::STONE:   t = VoxelType::STONE; break;
                                    case BlockType::DIRT:    t = VoxelType::DIRT; break;
                                    case BlockType::GRASS:   t = VoxelType::GRASS; break;
                                    case BlockType::WOOD:    t = VoxelType::WOOD; break;
                                    case BlockType::LEAVES:  t = VoxelType::LEAVES; break;
                                    case BlockType::BEDROCK: t = VoxelType::BEDROCK; break;
                                    case BlockType::COAL:    t = VoxelType::COAL; break;
                                    case BlockType::IRON:    t = VoxelType::IRON; break;
                                    case BlockType::GOLD:    t = VoxelType::GOLD; break;
                                    case BlockType::DIAMOND: t = VoxelType::DIAMOND; break;
                                    case BlockType::LAVA:    t = VoxelType::LAVA; break;
                                    case BlockType::SAND:    t = VoxelType::SAND; break;
                                    case BlockType::WATER:   t = VoxelType::WATER; break;
                                    default:                 t = VoxelType::AIR; break;
                                }
                                
                                subChunk.setVoxel(x, y, z, t);
                            }
                        }
                    }
                    
                    subChunk.meshDirty = true;
                    registry.addComponent(subChunkEntity, subChunk);
                    registry.addComponent(subChunkEntity, MeshComponent());
                    registry.addComponent(subChunkEntity, TransformComponent(glm::vec3(0.0f,0.0f,0.0f)));
                    
                    chunkManager.subChunks[subY] = subChunkEntity;
                    requestMesh(subChunkEntity);
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
                                    requestMesh(subID);
                                }
                            }
                        }
                    }
                }
            }
        }
        
    }

    EntityID getSubChunkAt(int x, int y, int z, Registry& registry) {
        if (y < 0 || y > 15) return 0;
        
        auto it = activeChunks.find({x, z});
        if (it != activeChunks.end()) {
            EntityID parent = it->second;
            if (parent != 0 && registry.hasComponent<ChunkComponent>(parent)) {
                return registry.getComponent<ChunkComponent>(parent).subChunks[y];
            }
        }
        return 0;
    }
};