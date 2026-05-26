#pragma once

#include "component.hpp"
#include "modules/terrain_gen/TerrainGenerator.hpp"
#include "ecs/entity.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <array>
#include <bitset>

constexpr size_t SUBCHUNK_SIZE_X = 16;
constexpr size_t SUBCHUNK_SIZE_Y = 16;
constexpr size_t SUBCHUNK_SIZE_Z = 16;
constexpr size_t SUBVOXEL_ARRAY_SIZE = SUBCHUNK_SIZE_X * SUBCHUNK_SIZE_Y * SUBCHUNK_SIZE_Z;

// Voxel types (uint8_t to save memory)
enum class VoxelType : uint8_t {
    AIR = 0,
    STONE = 1,
    DIRT = 2,
    GRASS = 3,
    WOOD = 4,
    LEAVES = 5,
    BEDROCK = 6,
    COAL = 7,
    IRON = 8,
    GOLD = 9,
    DIAMOND = 10,
    LAVA = 11,
    SAND = 12,
    WATER = 13
};

struct SubChunkVisibility {
    std::bitset<36> bits;

    SubChunkVisibility() {
        bits.set();
    }

    void setConnected(int _faceFrom, int _faceTo, int _connected) {
        int bit = _faceFrom * 6 + _faceTo;
        bits[bit] = _connected;
    }

    bool isConnected(int _faceFrom, int _faceTo) const {
        if (_faceFrom == -1) return true;
        int bit = _faceFrom * 6 + _faceTo;
        return bits[bit];
    }
};

inline bool isOpaque(VoxelType type) {
    return type != VoxelType::AIR && type != VoxelType::WATER && type != VoxelType::LAVA;
}

struct SubChunkComponent : public Component {
    std::vector<uint8_t> voxels;
    glm::ivec3 subChunkPosition = glm::ivec3(0);
    bool meshDirty = true;

    int solidBlockCount = 0;
    std::array<glm::vec3, SUBCHUNK_SIZE_X * SUBCHUNK_SIZE_Z> biomeColors;

    SubChunkVisibility visibility;

    SubChunkComponent() : voxels(SUBVOXEL_ARRAY_SIZE, 0) {
        biomeColors.fill(glm::vec3(1.0f));
    }

    explicit SubChunkComponent(const glm::ivec3& pos)
        : subChunkPosition(pos), voxels(SUBVOXEL_ARRAY_SIZE, 0) {
        biomeColors.fill(glm::vec3(1.0f));
    }

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
            
            VoxelType oldType = static_cast<VoxelType>(voxels[idx]);
            if (oldType == VoxelType::AIR && type != VoxelType::AIR) solidBlockCount++;
            else if (oldType != VoxelType::AIR && type == VoxelType::AIR) solidBlockCount--;

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
