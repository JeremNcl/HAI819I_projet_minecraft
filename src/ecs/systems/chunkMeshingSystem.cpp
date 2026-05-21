#include "chunkMeshingSystem.hpp"
#include <algorithm>

bool ChunkMeshingSystem::isVoxelSolid(const ChunkComponent& voxelData, int x, int y, int z) const {
    VoxelType type = voxelData.getVoxel(x, y, z);
    return type != VoxelType::AIR;
}

// Helper: Get color for voxel type
static glm::vec3 getVoxelColor(VoxelType type) {
    switch (type) {
        case VoxelType::STONE:
            return glm::vec3(0.5f, 0.5f, 0.5f);  // Gray
        case VoxelType::DIRT:
            return glm::vec3(0.6f, 0.4f, 0.2f);  // Brown
        case VoxelType::GRASS:
            return glm::vec3(0.2f, 0.7f, 0.2f);  // Green
        default:
            return glm::vec3(1.0f, 1.0f, 1.0f);  // White (fallback)
    }
}

void ChunkMeshingSystem::addFace(std::vector<Vertex>& vertices,
                                  std::vector<GLuint>& indices,
                                  glm::vec3 corner,
                                  glm::vec3 edge1,
                                  glm::vec3 edge2,
                                  glm::vec3 normal,
                                  glm::vec3 color) {
    GLuint baseIdx = static_cast<GLuint>(vertices.size());

    // Create quad (2 triangles) at corner with edges
    vertices.push_back({corner, normal, color});
    vertices.push_back({corner + edge1, normal, color});
    vertices.push_back({corner + edge1 + edge2, normal, color});
    vertices.push_back({corner + edge2, normal, color});

    // Triangle 1 (CCW winding)
    indices.push_back(baseIdx);
    indices.push_back(baseIdx + 1);
    indices.push_back(baseIdx + 2);

    // Triangle 2 (CCW winding)
    indices.push_back(baseIdx);
    indices.push_back(baseIdx + 2);
    indices.push_back(baseIdx + 3);
}

void ChunkMeshingSystem::generateMesh(Registry& registry, EntityID entity,
                                      ChunkComponent& voxelData,
                                      MeshComponent& mesh) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    // Iterate over all voxels in chunk
    for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
        for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
            for (int x = 0; x < CHUNK_SIZE_X; ++x) {
                if (!isVoxelSolid(voxelData, x, y, z)) {
                    continue;
                }

                VoxelType voxelType = voxelData.getVoxel(x, y, z);
                glm::vec3 voxelColor = getVoxelColor(voxelType);
                glm::vec3 voxelPos(x, y, z);

                // Check all 6 faces
                // +X face (viewed from outside, normal points +X)
                if (!isVoxelSolid(voxelData, x + 1, y, z)) {
                    addFace(vertices, indices,
                            voxelPos + glm::vec3(1, 0, 0),
                            glm::vec3(0, 0, 1),
                            glm::vec3(0, 1, 0),
                            glm::vec3(1, 0, 0),
                            voxelColor);
                }

                // -X face (viewed from outside, normal points -X)
                if (!isVoxelSolid(voxelData, x - 1, y, z)) {
                    addFace(vertices, indices,
                            voxelPos + glm::vec3(0, 0, 0),
                            glm::vec3(0, 0, 1),
                            glm::vec3(0, 1, 0),
                            glm::vec3(-1, 0, 0),
                            voxelColor);
                }

                // +Y face (viewed from outside, normal points +Y)
                if (!isVoxelSolid(voxelData, x, y + 1, z)) {
                    addFace(vertices, indices,
                            voxelPos + glm::vec3(0, 1, 0),
                            glm::vec3(1, 0, 0),
                            glm::vec3(0, 0, 1),
                            glm::vec3(0, 1, 0),
                            voxelColor);
                }

                // -Y face (viewed from outside, normal points -Y)
                if (!isVoxelSolid(voxelData, x, y - 1, z)) {
                    addFace(vertices, indices,
                            voxelPos + glm::vec3(0, 0, 0),
                            glm::vec3(0, 0, 1),
                            glm::vec3(1, 0, 0),
                            glm::vec3(0, -1, 0),
                            voxelColor);
                }

                // +Z face (viewed from outside, normal points +Z)
                if (!isVoxelSolid(voxelData, x, y, z + 1)) {
                    addFace(vertices, indices,
                            voxelPos + glm::vec3(0, 0, 1),
                            glm::vec3(1, 0, 0),
                            glm::vec3(0, 1, 0),
                            glm::vec3(0, 0, 1),
                            voxelColor);
                }

                // -Z face (viewed from outside, normal points -Z)
                if (!isVoxelSolid(voxelData, x, y, z - 1)) {
                    addFace(vertices, indices,
                            voxelPos + glm::vec3(0, 0, 0),
                            glm::vec3(1, 0, 0),
                            glm::vec3(0, 1, 0),
                            glm::vec3(0, 0, -1),
                            voxelColor);
                }
            }
        }
    }

    // Cleanup old mesh
    mesh.cleanup();

    if (vertices.empty()) {
        mesh.indexCount = 0;
        return;
    }

    // Create VAO, VBO, IBO
    glGenVertexArrays(1, &mesh.VAO);
    glBindVertexArray(mesh.VAO);

    // VBO
    glGenBuffers(1, &mesh.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Vertex attributes
    glEnableVertexAttribArray(0);  // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);  // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);  // color
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    // IBO
    glGenBuffers(1, &mesh.IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    mesh.indexCount = indices.size();

    glBindVertexArray(0);
}

void ChunkMeshingSystem::update(Registry& registry) {
    auto view = registry.view<ChunkComponent, MeshComponent>();

    for (EntityID entity : view) {
        auto& voxelData = registry.getComponent<ChunkComponent>(entity);

        if (voxelData.meshDirty) {
            auto& mesh = registry.getComponent<MeshComponent>(entity);
            generateMesh(registry, entity, voxelData, mesh);
            voxelData.meshDirty = false;
        }
    }
}
