#include "PathFinder3D.hpp"
#include <algorithm>
#include <iostream>

int PathFinder3D::GetDistance(glm::ivec3 a, glm::ivec3 b) const {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z);
}

std::vector<glm::ivec3> PathFinder3D::FindPath(glm::ivec3 startPos, glm::ivec3 targetPos, Registry& registry) {
    std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> openSet;
    std::unordered_map<glm::ivec3, int, GLMVec3Hash> gCostMap;
    std::unordered_map<glm::ivec3, glm::ivec3, GLMVec3Hash> parentMap;

    openSet.push({startPos, 0, GetDistance(startPos, targetPos), startPos});
    gCostMap[startPos] = 0;

    int maxIterations = 5000;
    int iterations = 0;

    while (!openSet.empty() && iterations < maxIterations) {
        iterations++;
        PathNode currentNode = openSet.top();
        openSet.pop();

        if (currentNode.pos == targetPos) {
            std::cout << "Chemin trouve en " << iterations << " iterations !\n";
            return RetracePath(parentMap, startPos, targetPos);
        }

        if (currentNode.gCost > gCostMap[currentNode.pos]) continue;

        for (glm::ivec3 neighborPos : GetValidNeighbors(currentNode.pos, registry)) {
            int newMovementCostToNeighbor = currentNode.gCost + 1;

            if (gCostMap.find(neighborPos) == gCostMap.end() || newMovementCostToNeighbor < gCostMap[neighborPos]) {
                gCostMap[neighborPos] = newMovementCostToNeighbor;
                parentMap[neighborPos] = currentNode.pos; 
                int hCost = GetDistance(neighborPos, targetPos);
                openSet.push({neighborPos, newMovementCostToNeighbor, hCost, currentNode.pos});
            }
        }
    }
    std::cout << "Aucun chemin trouve ! (Cible inaccessible)\n";
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

std::vector<glm::ivec3> PathFinder3D::GetValidNeighbors(glm::ivec3 currentPos, Registry& registry) {
    std::vector<glm::ivec3> neighbors;
    glm::ivec3 directions[6] = {
        glm::ivec3( 1,  0,  0), glm::ivec3(-1,  0,  0),
        glm::ivec3( 0,  1,  0), glm::ivec3( 0, -1,  0),
        glm::ivec3( 0,  0,  1), glm::ivec3( 0,  0, -1)
    };

    for (int i = 0; i < 6; ++i) {
        glm::ivec3 neighborPos = currentPos + directions[i];
        
        if (!IsBlockSolid(neighborPos, registry)) {
            neighbors.push_back(neighborPos);
        }
    }
    return neighbors;
}

bool PathFinder3D::IsBlockSolid(glm::ivec3 pos, Registry& registry) const {
    auto view = registry.view<ChunkComponent, MeshComponent>();
    
    for (EntityID entity : view) {
        const auto& chunk = registry.getComponent<ChunkComponent>(entity);
        
        int minX = chunk.chunkPosition.x * 16;
        int minZ = chunk.chunkPosition.z * 16;

        if (pos.x >= minX && pos.x < minX + 16 &&
            pos.z >= minZ && pos.z < minZ + 16 &&
            pos.y >= 0 && pos.y < 256) {
            
            VoxelType type = chunk.getVoxel(pos.x - minX, pos.y, pos.z - minZ);
            return type != VoxelType::AIR;
        }
    }
    
    return true; 
}
