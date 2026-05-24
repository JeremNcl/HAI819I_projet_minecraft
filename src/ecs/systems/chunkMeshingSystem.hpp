#pragma once

#include "../registry.hpp"
#include "../components/chunk.hpp"
#include "../components/mesh.hpp"
#include "../components/world.hpp"
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

    void update(Registry& registry, const WorldMapComponent& worldMap);

private:
    void generateMesh(Registry& registry, EntityID entity,
                      SubChunkComponent& voxelData,
                      MeshComponent& mesh,
                      const WorldMapComponent& worldMap);

    void addFace(std::vector<Vertex>& vertices,
                 std::vector<GLuint>& indices,
                 glm::vec3 corner,
                 glm::vec3 edge1,
                 glm::vec3 edge2,
                 glm::vec3 normal,
                 float texIndex,
                 int axis,
                 glm::vec3 worldOffset);

    VoxelType getVoxelGlobal(Registry& registry, const SubChunkComponent& voxelData, int x, int y, int z, const WorldMapComponent& worldMap) const;};
