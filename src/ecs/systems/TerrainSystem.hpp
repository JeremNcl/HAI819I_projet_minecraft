#pragma once

#include "ecs/registry.hpp"
#include "ecs/components/chunk.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/mesh.hpp"
#include "ecs/components/camera.hpp"
#include "modules/ChunkWorker.hpp"
#include "modules/ChunkSerializer.hpp"
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
    
    int renderDistance = 4;
    int unloadDistance = 6;

    void destroyChunkRecursive(Registry& registry, EntityID parentEntity) {
        if (registry.hasComponent<ChunkComponent>(parentEntity)) {
            auto& chunkComp = registry.getComponent<ChunkComponent>(parentEntity);
            
            if (chunkComp.isModified) {
                ChunkDataDTO dto;
                dto.x = chunkComp.chunkPosition.x;
                dto.z = chunkComp.chunkPosition.y;
                dto.subChunkMask = 0;

                bool biomesCopied = false;

                for (int subY = 0; subY < 16; ++subY) {
                    EntityID subChunkID = chunkComp.subChunks[subY];
                    if (subChunkID != 0 && registry.hasComponent<SubChunkComponent>(subChunkID)) {
                        auto& subChunk = registry.getComponent<SubChunkComponent>(subChunkID);
                        
                        if (!biomesCopied) {
                            dto.biomeColors = subChunk.biomeColors;
                            biomesCopied = true;
                        }

                        if (subChunk.solidBlockCount > 0) {
                            dto.subChunkMask |= (1 << subY);
                            dto.subChunksVoxels[subY] = subChunk.voxels;
                        }
                    }
                }
                
                ChunkSerializer::SaveChunk(dto);
            }
            
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
                chunkManager.isModified = false;

                for (int subY = 0; subY < 16; ++subY){
                    EntityID subChunkEntity = registry.createEntity();
                    SubChunkComponent subChunk(glm::ivec3(result.x, subY, result.z));
                    subChunk.biomeColors = result.chunkData.biomeColors;
                    
                    if (result.chunkData.subChunkMask & (1 << subY)) {
                        subChunk.voxels = std::move(result.chunkData.subChunksVoxels[subY]);
                        
                        int solidCount = 0;
                        for (uint8_t v : subChunk.voxels) {
                            if (static_cast<VoxelType>(v) != VoxelType::AIR) solidCount++;
                        }
                        subChunk.solidBlockCount = solidCount;
                        subChunk.meshDirty = (solidCount > 0);
                    } else {
                        subChunk.voxels.assign(4096, 0);
                        subChunk.solidBlockCount = 0;
                        subChunk.meshDirty = false;
                    }
                    
                    registry.addComponent(subChunkEntity, subChunk);
                    registry.addComponent(subChunkEntity, MeshComponent());
                    registry.addComponent(subChunkEntity, TransformComponent(glm::vec3(0.0f,0.0f,0.0f)));
                    
                    chunkManager.subChunks[subY] = subChunkEntity;
                    
                    if (subChunk.solidBlockCount > 0) {
                        requestMesh(subChunkEntity);
                    }
                }

                chunkManager.isFullyGenerated = true;
                registry.addComponent(parentEntity, chunkManager);

                std::pair<int, int> neighbors[4] = {
                    {result.chunkData.x + 1, result.chunkData.z}, {result.chunkData.x - 1, result.chunkData.z},
                    {result.chunkData.x, result.chunkData.z + 1}, {result.chunkData.x, result.chunkData.z - 1}
                };
                
                for (const auto& n : neighbors) {
                    if (activeChunks.find(n) != activeChunks.end()) {
                        EntityID neighborParent = activeChunks[n];
                        if (neighborParent != 0 && registry.hasComponent<ChunkComponent>(neighborParent)) {
                            auto& neighborChunk = registry.getComponent<ChunkComponent>(neighborParent);
                            
                            for (int subY = 0; subY < 16; ++subY) {
                                EntityID mySubID = chunkManager.subChunks[subY];
                                EntityID neighborSubID = neighborChunk.subChunks[subY];

                                if (mySubID != 0 && neighborSubID != 0) {
                                    auto& mySub = registry.getComponent<SubChunkComponent>(mySubID);
                                    auto& neighborSub = registry.getComponent<SubChunkComponent>(neighborSubID);
                                    
                                    if (mySub.solidBlockCount > 0 && neighborSub.solidBlockCount > 0 && !neighborSub.meshDirty) {
                                        neighborSub.meshDirty = true;
                                        requestMesh(neighborSubID);
                                    }
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

    void setBlock(Registry& registry, int globalX, int globalY, int globalZ, VoxelType type) {
    
        if (globalY < 0 || globalY >= 256) return;
        
        int chunkX = static_cast<int>(std::floor(globalX / 16.0f));
        int chunkZ = static_cast<int>(std::floor(globalZ / 16.0f));
        int subY = globalY / 16;

        int localX = globalX - (chunkX * 16);
        int localY = globalY % 16;
        int localZ = globalZ - (chunkZ * 16);

        EntityID mySubID = getSubChunkAt(chunkX, subY, chunkZ, registry);
        if (mySubID == 0) return;

        auto& mySub = registry.getComponent<SubChunkComponent>(mySubID);

        VoxelType oldType = mySub.getVoxel(localX, localY, localZ);
    
        if (oldType == type) return;

        if (oldType != VoxelType::AIR && type == VoxelType::AIR) {
            mySub.solidBlockCount--;
        } else if (oldType == VoxelType::AIR && type != VoxelType::AIR) {
            mySub.solidBlockCount++;
        }

        mySub.setVoxel(localX, localY, localZ, type);
        
        auto itChunk = activeChunks.find({chunkX, chunkZ});
        if (itChunk != activeChunks.end()) {
            EntityID parentEntity = itChunk->second;
            if (parentEntity != 0 && registry.hasComponent<ChunkComponent>(parentEntity)) {
                auto& chunkComp = registry.getComponent<ChunkComponent>(parentEntity);
                chunkComp.isModified = true;
            }
        }

        if (!mySub.meshDirty) {
            mySub.meshDirty = true;
            requestMesh(mySubID);
        }

        auto dirtyNeighbor = [&](int nX, int nSubY, int nZ) {
            EntityID nID = getSubChunkAt(nX, nSubY, nZ, registry);
            if (nID != 0) {
                auto& nSub = registry.getComponent<SubChunkComponent>(nID);
                if (!nSub.meshDirty) {
                    nSub.meshDirty = true;
                    requestMesh(nID);
                }
            }
        };

        if (localX == 0) dirtyNeighbor(chunkX - 1, subY, chunkZ);
        if (localX == 15) dirtyNeighbor(chunkX + 1, subY, chunkZ);

        if (localZ == 0) dirtyNeighbor(chunkX, subY, chunkZ - 1);
        if (localZ == 15) dirtyNeighbor(chunkX, subY, chunkZ + 1);

        if (localY == 0 && subY > 0) dirtyNeighbor(chunkX, subY - 1, chunkZ);
        if (localY == 15 && subY < 15) dirtyNeighbor(chunkX, subY + 1, chunkZ);
    }
};