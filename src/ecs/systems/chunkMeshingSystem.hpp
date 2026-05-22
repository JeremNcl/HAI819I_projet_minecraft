#pragma once

#include "../registry.hpp"
#include "../components/chunk.hpp"
#include "../components/mesh.hpp"
#include "../components/transform.hpp"
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>
#include <unordered_map>


#ifndef GLM_VEC3_HASH_DEFINED
#define GLM_VEC3_HASH_DEFINED

struct GLMVec3Hash {
    std::size_t operator()(const glm::ivec3& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1) ^ (std::hash<int>()(k.z) << 2);
    }
};
#endif

using SubChunkCache = std::unordered_map<glm::ivec3, const SubChunkComponent*, GLMVec3Hash>;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 texCoords;
};

class ChunkMeshingSystem {
public:
    ChunkMeshingSystem() = default;

    void update(Registry& registry);

private:
    void generateMesh(Registry& registry, EntityID entity,
                      SubChunkComponent& voxelData,
                      MeshComponent& mesh,
                      const SubChunkCache& cache);

    void addFace(std::vector<Vertex>& vertices,
                 std::vector<GLuint>& indices,
                 glm::vec3 corner,
                 glm::vec3 edge1,
                 glm::vec3 edge2,
                 glm::vec3 normal,
                 float texIndex,
                 int axis,
                 glm::vec3 worldOffset);

    VoxelType getVoxelGlobal(const SubChunkComponent& voxelData, int x, int y, int z, const SubChunkCache& cache) const;};
