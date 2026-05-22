#pragma once

#include "component.hpp"
#include "../../modules/terrain_gen/TerrainGenerator.hpp"
#include "../entity.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <array>

constexpr size_t SUBCHUNK_SIZE_X = 16;
constexpr size_t SUBCHUNK_SIZE_Y = 16;
constexpr size_t SUBCHUNK_SIZE_Z = 16;
constexpr size_t SUBVOXEL_ARRAY_SIZE = SUBCHUNK_SIZE_X * SUBCHUNK_SIZE_Y * SUBCHUNK_SIZE_Z;

// Voxel types (uint8_t to save memory)
enum class VoxelType : uint8_t {
    AIR = 0,
    STONE = 1,
    DIRT = 2,
    GRASS = 3
};

struct SubChunkComponent : public Component {
    std::vector<uint8_t> voxels;
    glm::ivec3 subChunkPosition = glm::ivec3(0);
    bool meshDirty = true;

    SubChunkComponent() : voxels(SUBVOXEL_ARRAY_SIZE, 0) {}

    explicit SubChunkComponent(const glm::ivec3& pos)
        : subChunkPosition(pos), voxels(SUBVOXEL_ARRAY_SIZE, 0) {}

    inline size_t getIndex(int x, int y, int z) const {
        if (x < 0 || x >= SUBCHUNK_SIZE_X ||
            y < 0 || y >= SUBCHUNK_SIZE_Y ||
            z < 0 || z >= SUBCHUNK_SIZE_Z) {
            return SUBVOXEL_ARRAY_SIZE;
        }
        return x + (y * SUBCHUNK_SIZE_X * SUBCHUNK_SIZE_Z) + (z * SUBCHUNK_SIZE_X);
    }

    inline VoxelType getVoxel(int x, int y, int z) const {
        size_t idx = getIndex(x, y, z);
        if (idx >= SUBVOXEL_ARRAY_SIZE) return VoxelType::AIR;
        return static_cast<VoxelType>(voxels[idx]);
    }

    inline void setVoxel(int x, int y, int z, VoxelType type) {
        size_t idx = getIndex(x, y, z);
        if (idx < SUBVOXEL_ARRAY_SIZE) {
            voxels[idx] = static_cast<uint8_t>(type);
        }
    }
};

struct ChunkComponent : public Component {
    glm::ivec2 chunkPosition = glm::ivec2(0); 
    
    std::array<EntityID, 16> subChunks = {0}; 
    
    bool isFullyGenerated = false;

    ChunkComponent() = default;

    explicit ChunkComponent(const glm::ivec2& pos)
        : chunkPosition(pos) {}
};
