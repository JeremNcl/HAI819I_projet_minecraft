#include "chunkMeshingSystem.hpp"
#include "../../engine/math/TBNValidator.hpp"
#include <algorithm>

bool ChunkMeshingSystem::isVoxelSolid(const ChunkComponent& voxelData, int x, int y, int z) const {
    VoxelType type = voxelData.getVoxel(x, y, z);
    return type != VoxelType::AIR;
}

static float getTextureIndex(VoxelType type, int axis, bool isPositive) {
    switch (type) {
        case VoxelType::STONE: return 0.0f; // Index 0 dans le Texture Array
        case VoxelType::DIRT:  return 1.0f; // Index 1 dans le Texture Array
        case VoxelType::GRASS: return 2.0f; // Herbe pleine (2)
        default: return 0.0f;
    }
}

void ChunkMeshingSystem::addFace(std::vector<Vertex>& vertices,
                                  std::vector<GLuint>& indices,
                                  glm::vec3 corner,
                                  glm::vec3 edge1,
                                  glm::vec3 edge2,
                                  glm::vec3 normal,
                                  float texIndex,
                                  int axis) {
    GLuint baseIdx = static_cast<GLuint>(vertices.size());

    auto getUV = [axis](glm::vec3 pos) -> glm::vec2 {
        if (axis == 0) return glm::vec2(pos.z, pos.y);
        if (axis == 1) return glm::vec2(pos.x, pos.z);
        return glm::vec2(pos.x, pos.y);
    };

    glm::vec3 p0 = corner;
    glm::vec3 p1 = corner + edge1;
    glm::vec3 p2 = corner + edge1 + edge2;
    glm::vec3 p3 = corner + edge2;

    glm::vec3 tangent = glm::normalize(edge1);
    glm::vec3 bitangent = glm::normalize(edge2);

    if constexpr (false) {
        TBNValidator::validateTBN(tangent, bitangent, normal);
    }

    vertices.push_back({p0, normal, glm::vec3(getUV(p0), texIndex), tangent, bitangent});
    vertices.push_back({p1, normal, glm::vec3(getUV(p1), texIndex), tangent, bitangent});
    vertices.push_back({p2, normal, glm::vec3(getUV(p2), texIndex), tangent, bitangent});
    vertices.push_back({p3, normal, glm::vec3(getUV(p3), texIndex), tangent, bitangent});

    indices.push_back(baseIdx);
    indices.push_back(baseIdx + 1);
    indices.push_back(baseIdx + 2);

    indices.push_back(baseIdx);
    indices.push_back(baseIdx + 2);
    indices.push_back(baseIdx + 3);
}

void ChunkMeshingSystem::generateMesh(Registry& registry, EntityID entity,
                                      ChunkComponent& voxelData,
                                      MeshComponent& mesh) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    int dims[3] = {CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z};

    for (int axis = 0; axis < 3; ++axis) {
        int u = (axis + 1) % 3;
        int v = (axis + 2) % 3;
        int x[3] = {0, 0, 0};
        int q[3] = {0, 0, 0};
        q[axis] = 1;

        for (int dir = 0; dir < 2; ++dir) {
            bool isPositive = (dir == 1);
            int normalDir = isPositive ? 1 : -1;

            glm::vec3 normal(0.0f);
            normal[axis] = normalDir;
            std::vector<VoxelType> mask(dims[u] * dims[v], VoxelType::AIR);

            for (x[axis] = 0; x[axis] < dims[axis]; ++x[axis]) {
                for (x[v] = 0; x[v] < dims[v]; ++x[v]) {
                    for (x[u] = 0; x[u] < dims[u]; ++x[u]) {
                        VoxelType current = voxelData.getVoxel(x[0], x[1], x[2]);

                        if (current != VoxelType::AIR) {
                            int nx = x[0] + (isPositive ? q[0] : -q[0]);
                            int ny = x[1] + (isPositive ? q[1] : -q[1]);
                            int nz = x[2] + (isPositive ? q[2] : -q[2]);

                            VoxelType neighbor = voxelData.getVoxel(nx, ny, nz);
                            if (neighbor == VoxelType::AIR) {
                                mask[x[u] + x[v] * dims[u]] = current;
                            } else {
                                mask[x[u] + x[v] * dims[u]] = VoxelType::AIR;
                            }
                        } else {
                            mask[x[u] + x[v] * dims[u]] = VoxelType::AIR;
                        }
                    }
                }

                for (int j = 0; j < dims[v]; ++j) {
                    for (int i = 0; i < dims[u]; ) {
                        VoxelType type = mask[i + j * dims[u]];
                        if (type != VoxelType::AIR) {
                            int w = 1;
                            while (i + w < dims[u] && mask[(i + w) + j * dims[u]] == type) {
                                w++;
                            }

                            int h = 1;
                            bool done = false;
                            while (j + h < dims[v]) {
                                for (int k = 0; k < w; ++k) {
                                    if (mask[(i + k) + (j + h) * dims[u]] != type) {
                                        done = true;
                                        break;
                                    }
                                }
                                if (done) break;
                                h++;
                            }

                            x[u] = i;
                            x[v] = j;

                            int du[3] = {0, 0, 0}; du[u] = w;
                            int dv[3] = {0, 0, 0}; dv[v] = h;

                            glm::vec3 corner(x[0], x[1], x[2]);
                            if (isPositive) corner[axis] += 1.0f;

                            glm::vec3 edge1(du[0], du[1], du[2]);
                            glm::vec3 edge2(dv[0], dv[1], dv[2]);

                            float texIndex = getTextureIndex(type, axis, isPositive);

                            glm::vec3 calculatedNormal = glm::normalize(glm::cross(edge1, edge2));
                            if (glm::dot(calculatedNormal, normal) < 0) {
                                addFace(vertices, indices, corner, edge2, edge1, normal, texIndex, axis);
                            } else {
                                addFace(vertices, indices, corner, edge1, edge2, normal, texIndex, axis);
                            }

                            for (int l = 0; l < h; ++l) {
                                for (int k = 0; k < w; ++k) {
                                    mask[(i + k) + (j + l) * dims[u]] = VoxelType::AIR;
                                }
                            }
                            i += w;
                        } else {
                            i++;
                        }
                    }
                }
            }
        }
    }

    mesh.cleanup();
    if (vertices.empty()) {
        mesh.indexCount = 0;
        return;
    }

    glGenVertexArrays(1, &mesh.VAO);
    glBindVertexArray(mesh.VAO);

    glGenBuffers(1, &mesh.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);  
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);  
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

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

void ChunkMeshingSystem::validateTBNIntegrity() {
    std::cout << "Validating TBN Integrity..." << std::endl;
    
    std::vector<glm::vec3> testCases[3] = {
        {glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)},
        {glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), glm::vec3(1, 0, 0)},
        {glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0)},
    };
    
    int passed = 0, failed = 0;
    
    for (int i = 0; i < 3; i++) {
        if (TBNValidator::validateTBN(testCases[i][0], testCases[i][1], testCases[i][2])) {
            passed++;
        } else {
            failed++;
        }
    }
    
    std::cout << "TBN Validation Results: " << passed << " passed, " << failed << " failed" << std::endl;
}
