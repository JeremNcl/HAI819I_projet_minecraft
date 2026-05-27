#include "PathFinder3D.hpp"
#include <algorithm>
#include <iostream>

float PathFinder3D::GetDistance(glm::ivec3 a, glm::ivec3 b) const {
    return glm::distance(glm::vec3(a), glm::vec3(b));
}

std::vector<glm::ivec3> PathFinder3D::FindPath(glm::ivec3 startPos, glm::ivec3 targetPos, Registry& registry) {
    SubChunkCache subChunkCache;
    auto view = registry.view<SubChunkComponent, MeshComponent>();
    for (EntityID entity : view) {
        const auto& subChunk = registry.getComponent<SubChunkComponent>(entity);
        subChunkCache[subChunk.subChunkPosition] = &subChunk;
    }

    std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> openSet;
    std::unordered_map<glm::ivec3, float, GLMVec3Hash> gCostMap;
    std::unordered_map<glm::ivec3, glm::ivec3, GLMVec3Hash> parentMap;

    openSet.push({startPos, 0.0f, GetDistance(startPos, targetPos), startPos});
    gCostMap[startPos] = 0.0f;

    int maxIterations = 3000;
    int iterations = 0;

    while (!openSet.empty() && iterations < maxIterations) {
        iterations++;
        PathNode currentNode = openSet.top();
        openSet.pop();

        if (currentNode.pos == targetPos) {
            return RetracePath(parentMap, startPos, targetPos);
        }

        if (currentNode.gCost > gCostMap[currentNode.pos]) continue;

        for (glm::ivec3 neighborPos : GetValidNeighbors(currentNode.pos, subChunkCache)) {
            float newMovementCostToNeighbor = currentNode.gCost + GetDistance(currentNode.pos, neighborPos);

            if (gCostMap.find(neighborPos) == gCostMap.end() || newMovementCostToNeighbor < gCostMap[neighborPos]) {
                gCostMap[neighborPos] = newMovementCostToNeighbor;
                parentMap[neighborPos] = currentNode.pos; 
                float hCost = GetDistance(neighborPos, targetPos);
                openSet.push({neighborPos, newMovementCostToNeighbor, hCost, currentNode.pos});
            }
        }
    }
    return std::vector<glm::ivec3>(); 
}

std::vector<glm::ivec3> PathFinder3D::RetracePath(std::unordered_map<glm::ivec3, glm::ivec3, GLMVec3Hash>& parentMap, glm::ivec3 start, glm::ivec3 end) {
    std::vector<glm::ivec3> path;
    glm::ivec3 current = end;
    while (current != start) {
        path.push_back(current);
        current = parentMap[current]; 
    }
    std::reverse(path.begin(), path.end()); 
    return path;
}

std::vector<glm::ivec3> PathFinder3D::GetValidNeighbors(glm::ivec3 currentPos, const SubChunkCache& subChunkCache) {
    std::vector<glm::ivec3> neighbors;

    glm::ivec3 horizontalDirs[8] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1},
        {1, 0, 1}, {1, 0, -1}, {-1, 0, 1}, {-1, 0, -1}
    };

    for (int i = 0; i < 8; ++i) {
        glm::ivec3 dir = horizontalDirs[i];

        if (std::abs(dir.x) == 1 && std::abs(dir.z) == 1) {
            if (IsBlockSolid(currentPos + glm::ivec3(dir.x, 0, 0), subChunkCache) ||
                IsBlockSolid(currentPos + glm::ivec3(0, 0, dir.z), subChunkCache)) {
                continue; 
            }
        }

        glm::ivec3 flatTarget = currentPos + dir;
        if (!IsBlockSolid(flatTarget, subChunkCache) && 
            !IsBlockSolid(flatTarget + glm::ivec3(0, 1, 0), subChunkCache)) {
            
            if (IsBlockSolid(flatTarget + glm::ivec3(0, -1, 0), subChunkCache)) {
                neighbors.push_back(flatTarget);
                continue;
            }
        }

        glm::ivec3 upTarget = currentPos + dir + glm::ivec3(0, 1, 0);
        if (IsBlockSolid(currentPos + dir, subChunkCache) &&                              
            !IsBlockSolid(upTarget, subChunkCache) &&                                       
            !IsBlockSolid(upTarget + glm::ivec3(0, 1, 0), subChunkCache) &&              
            !IsBlockSolid(currentPos + glm::ivec3(0, 2, 0), subChunkCache)) {            
            
            neighbors.push_back(upTarget);
            continue;
        }

        glm::ivec3 downTarget = currentPos + dir + glm::ivec3(0, -1, 0);
        if (!IsBlockSolid(currentPos + dir, subChunkCache) &&                          
            !IsBlockSolid(currentPos + dir + glm::ivec3(0, 1, 0), subChunkCache) &&         
            !IsBlockSolid(downTarget, subChunkCache) &&                            
            IsBlockSolid(downTarget + glm::ivec3(0, -1, 0), subChunkCache)) {    
            
            neighbors.push_back(downTarget);
            continue;
        }
    }

    return neighbors;
}

bool PathFinder3D::IsBlockSolid(glm::ivec3 pos, const SubChunkCache& subChunkCache) const {
    if (pos.y < 0 || pos.y >= 256) return true; 
    int chunkX = (pos.x >= 0) ? (pos.x / 16) : ((pos.x + 1) / 16) - 1;
    int chunkZ = (pos.z >= 0) ? (pos.z / 16) : ((pos.z + 1) / 16) - 1;
    int subY   = pos.y / 16;

    auto it = subChunkCache.find(glm::ivec3(chunkX, subY, chunkZ));
    if (it != subChunkCache.end()) {
        const SubChunkComponent* subChunk = it->second;
        int localX = pos.x - (chunkX * 16);
        int localZ = pos.z - (chunkZ * 16);

        VoxelType type = subChunk->getVoxel(localX, pos.y % 16, localZ);
        return type != VoxelType::AIR;
    }
    return true; 
}