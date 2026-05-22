#pragma once

#include "../registry.hpp"
#include "../components/chunk.hpp"
#include "../components/mesh.hpp"
#include "../components/transform.hpp"
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>

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
                      ChunkComponent& voxelData,
                      MeshComponent& mesh);

    void addFace(std::vector<Vertex>& vertices,
                 std::vector<GLuint>& indices,
                 glm::vec3 corner,
                 glm::vec3 edge1,
                 glm::vec3 edge2,
                 glm::vec3 normal,
                 float texIndex,
                 int axis);

    bool isVoxelSolid(const ChunkComponent& voxelData, int x, int y, int z) const;
};
