#pragma once
#ifndef WORLD_HPP
#define WORLD_HPP

#include "../entity.hpp"
#include "chunk.hpp"
#include <unordered_map>
#include <glm/glm.hpp>
#include <iostream>

#ifndef GLM_VEC3_HASH_DEFINED
#define GLM_VEC3_HASH_DEFINED

struct GLMVec3Hash {
    std::size_t operator()(const glm::ivec3& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1) ^ (std::hash<int>()(k.z) << 2);
    }
};
#endif

struct WorldMapComponent : public Component {
    std::unordered_map<glm::ivec3, EntityID, GLMVec3Hash> subChunkEntities;

    inline bool isVoxelSolid(Registry& _registry, int _x, int _y, int _z) const {
        glm::ivec3 subChunkPos(
            static_cast<int>(std::floor(static_cast<float>(_x) / static_cast<float>(SUBCHUNK_SIZE_X))),
            static_cast<int>(std::floor(static_cast<float>(_y) / static_cast<float>(SUBCHUNK_SIZE_Y))),
            static_cast<int>(std::floor(static_cast<float>(_z) / static_cast<float>(SUBCHUNK_SIZE_Z)))
        );

        int localX = ((_x % SUBCHUNK_SIZE_X) + SUBCHUNK_SIZE_X) % SUBCHUNK_SIZE_X;
        int localY = ((_y % SUBCHUNK_SIZE_Y) + SUBCHUNK_SIZE_Y) % SUBCHUNK_SIZE_Y;
        int localZ = ((_z % SUBCHUNK_SIZE_Z) + SUBCHUNK_SIZE_Z) % SUBCHUNK_SIZE_Z;

        auto it = subChunkEntities.find(subChunkPos);
        if (it != subChunkEntities.end()) {
            const SubChunkComponent& subChunk = _registry.getComponent<SubChunkComponent>(it->second);
            VoxelType type = subChunk.getVoxel(localX, localY, localZ);
            
            return type != VoxelType::AIR;
        }

        return false; 
    }
};
#endif