#pragma once

#include "ecs/registry.hpp"
#include "../components/world.hpp"
#include "ecs/components/chunk.hpp"
#include "ecs/components/mesh.hpp"
#include "ecs/components/transform.hpp"
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>
#include <queue>
#include <mutex>

using subChunkCache = std::vector<const SubChunkComponent*>;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 texCoords;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
};

struct MeshResult {
    EntityID entity;
    MeshData data;
};

class ChunkMeshingSystem {
public:
    ChunkMeshingSystem() = default;

    void update(Registry& registry, const WorldMapComponent& worldMap);
    bool isMeshingComplete(Registry& registry) const;
    int getCompletedMeshCount(Registry& registry) const;

    std::queue<MeshResult> uploadQueue;
    std::mutex uploadMutex;

    MeshData calculateMeshData(Registry& registry, EntityID entity,
                               SubChunkComponent& voxelData,
                               const subChunkCache& cache);
                               
    void uploadMeshToGPU(MeshComponent& mesh, const MeshData& data);

private:

    void addFace(std::vector<Vertex>& vertices,
                 std::vector<GLuint>& indices,
                 glm::vec3 corner,
                 glm::vec3 edge1,
                 glm::vec3 edge2,
                 glm::vec3 normal,
                 float texIndex,
                 int axis,
                 glm::vec3 worldOffset);

    VoxelType getVoxelGlobal(Registry& registry, const SubChunkComponent& voxelData, int x, int y, int z, const subChunkCache& cache) const;};
