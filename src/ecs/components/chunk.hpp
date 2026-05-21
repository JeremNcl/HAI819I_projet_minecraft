#pragma once

#include "component.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

constexpr size_t CHUNK_SIZE_X = 16;
constexpr size_t CHUNK_SIZE_Y = 256;
constexpr size_t CHUNK_SIZE_Z = 16;
constexpr size_t VOXEL_ARRAY_SIZE = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

// Voxel types (uint8_t to save memory)
enum class VoxelType : uint8_t {
    AIR = 0,
    STONE = 1,
    DIRT = 2,
    GRASS = 3
};

struct ChunkComponent : public Component {
    std::vector<uint8_t> voxels;
    glm::ivec3 chunkPosition = glm::ivec3(0);
    bool meshDirty = true;

    ChunkComponent() : voxels(VOXEL_ARRAY_SIZE, 0) {}

    explicit ChunkComponent(const glm::ivec3& pos)
        : chunkPosition(pos), voxels(VOXEL_ARRAY_SIZE, 0) {}

    // Helper: linear index from 3D coordinates
    inline size_t getIndex(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_SIZE_X ||
            y < 0 || y >= CHUNK_SIZE_Y ||
            z < 0 || z >= CHUNK_SIZE_Z) {
            return VOXEL_ARRAY_SIZE;  // out of bounds
        }
        return x + (y * CHUNK_SIZE_X * CHUNK_SIZE_Z) + (z * CHUNK_SIZE_X);
    }

    inline VoxelType getVoxel(int x, int y, int z) const {
        size_t idx = getIndex(x, y, z);
        if (idx >= VOXEL_ARRAY_SIZE) return VoxelType::AIR;
        return static_cast<VoxelType>(voxels[idx]);
    }

    inline void setVoxel(int x, int y, int z, VoxelType type) {
        size_t idx = getIndex(x, y, z);
        if (idx < VOXEL_ARRAY_SIZE) {
            voxels[idx] = static_cast<uint8_t>(type);
        }
    }
};
