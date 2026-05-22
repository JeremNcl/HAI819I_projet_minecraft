#pragma once
#include <vector>
#include <unordered_map>
#include <queue>
#include <cmath>
#include <glm/glm.hpp>
#include "../terrain_gen/TerrainGenerator.hpp"
#include "../../ecs/registry.hpp"
#include "../../ecs/components/chunk.hpp"
#include "../../ecs/components/mesh.hpp"

#ifndef GLM_VEC3_HASH_DEFINED
#define GLM_VEC3_HASH_DEFINED
struct GLMVec3Hash {
    std::size_t operator()(const glm::ivec3& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1) ^ (std::hash<int>()(k.z) << 2);
    }
};
#endif

using SubChunkCache = std::unordered_map<glm::ivec3, const SubChunkComponent*, GLMVec3Hash>;

struct GLMVec2Hash {
    std::size_t operator()(const glm::ivec2& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1);
    }
};

using ChunkCache = std::unordered_map<glm::ivec2, const ChunkComponent*, GLMVec2Hash>;

struct PathNode {
    glm::ivec3 pos;
    int gCost;
    int hCost;
    glm::ivec3 parent;

    int GetFCost() const { return gCost + hCost; }

    bool operator>(const PathNode& other) const {
        if (GetFCost() == other.GetFCost()) return hCost > other.hCost;
        return GetFCost() > other.GetFCost();
    }
};

class PathFinder3D {
public:
    std::vector<glm::ivec3> FindPath(glm::ivec3 startPos, glm::ivec3 targetPos, Registry& registry);
    
private:
    int GetDistance(glm::ivec3 a, glm::ivec3 b) const;
    std::vector<glm::ivec3> GetValidNeighbors(glm::ivec3 currentPos, const SubChunkCache& chunkCache);
    bool IsBlockSolid(glm::ivec3 pos, const SubChunkCache& chunkCache) const;
    std::vector<glm::ivec3> RetracePath(std::unordered_map<glm::ivec3, glm::ivec3, GLMVec3Hash>& parentMap, glm::ivec3 start, glm::ivec3 end);
};
