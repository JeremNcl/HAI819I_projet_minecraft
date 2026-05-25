#include "chunkMeshingSystem.hpp"
#include <algorithm>
#include <cmath>
#include "engine/render/BlockTextureManager.hpp"

VoxelType ChunkMeshingSystem::getVoxelGlobal(const SubChunkComponent& voxelData, int x, int y, int z, const subChunkCache& cache) const {
    if (x >= 0 && x < 16 && y >= 0 && y < 16 && z >= 0 && z < 16) {
        return voxelData.getVoxel(x, y, z);
    }

    glm::ivec3 neighborPos = voxelData.subChunkPosition;
    int localX = x;
    int localY = y;
    int localZ = z;

    if (x < 0) { neighborPos.x -= 1; localX = 15; }
    else if (x >= 16) { neighborPos.x += 1; localX = 0; }

    if (y < 0) { neighborPos.y -= 1; localY = 15; }
    else if (y >= 16) { neighborPos.y += 1; localY = 0; }

    if (z < 0) { neighborPos.z -= 1; localZ = 15; }
    else if (z >= 16) { neighborPos.z += 1; localZ = 0; }

    /* auto it = worldMap.subChunkEntities.find(neighborPos);
    if (it != worldMap.subChunkEntities.end()) {
        const SubChunkComponent& neighborComponent = registry.getComponent<SubChunkComponent>(it->second);
        return neighborComponent.getVoxel(localX, localY, localZ);
 */
    auto it = std::lower_bound(cache.begin(), cache.end(), neighborPos, [](const SubChunkComponent* comp, const glm::ivec3& pos) {
        if (comp->subChunkPosition.x != pos.x) return comp->subChunkPosition.x < pos.x;
        if (comp->subChunkPosition.y != pos.y) return comp->subChunkPosition.y < pos.y;
        return comp->subChunkPosition.z < pos.z;
    });

    if (it != cache.end() && (*it)->subChunkPosition == neighborPos) {
        return (*it)->getVoxel(localX, localY, localZ);
    }

    return VoxelType::AIR;
}

glm::vec3 ChunkMeshingSystem::sampleBiomeColor(const SubChunkComponent& voxelData, const glm::vec3& localPos) const {
    int x = static_cast<int>(std::round(localPos.x));
    int z = static_cast<int>(std::round(localPos.z));

    x = std::clamp(x, 0, 15);
    z = std::clamp(z, 0, 15);

    return voxelData.biomeColors[x + z * 16];
}


void ChunkMeshingSystem::addFace(std::vector<Vertex>& vertices,
                                  std::vector<GLuint>& indices,
                                  glm::vec3 corner,
                                  glm::vec3 edge1,
                                  glm::vec3 edge2,
                                  glm::vec3 normal,
                                  float texIndex,
                                  int axis,
                                  bool isPositive,
                                  glm::vec3 worldOffset,
                                  const SubChunkComponent& voxelData) {
    GLuint baseIdx = static_cast<GLuint>(vertices.size());

    auto getUV = [axis, isPositive](glm::vec3 localPos) -> glm::vec2 {
        if (axis == 0) {
            return isPositive ? glm::vec2(localPos.z, localPos.y)
                              : glm::vec2(1.0f - localPos.z, localPos.y);
        }

        if (axis == 1) {
            return isPositive ? glm::vec2(localPos.x, 1.0f - localPos.z)
                              : glm::vec2(localPos.x, localPos.z);
        }

        return isPositive ? glm::vec2(1.0f - localPos.x, localPos.y)
                          : glm::vec2(localPos.x, localPos.y);
    };

    glm::vec3 local_p0 = corner;
    glm::vec3 local_p1 = corner + edge1;
    glm::vec3 local_p2 = corner + edge1 + edge2;
    glm::vec3 local_p3 = corner + edge2;

    glm::vec3 p0 = local_p0 + worldOffset;
    glm::vec3 p1 = local_p1 + worldOffset;
    glm::vec3 p2 = local_p2 + worldOffset;
    glm::vec3 p3 = local_p3 + worldOffset;

    glm::vec3 biome0 = sampleBiomeColor(voxelData, local_p0);
    glm::vec3 biome1 = sampleBiomeColor(voxelData, local_p1);
    glm::vec3 biome2 = sampleBiomeColor(voxelData, local_p2);
    glm::vec3 biome3 = sampleBiomeColor(voxelData, local_p3);

    glm::vec2 uv0 = getUV(local_p0);
    glm::vec2 uv1 = getUV(local_p1);
    glm::vec2 uv2 = getUV(local_p2);
    glm::vec2 uv3 = getUV(local_p3);

    glm::vec3 deltaPos1 = p1 - p0;
    glm::vec3 deltaPos2 = p2 - p0;
    glm::vec2 deltaUV1 = uv1 - uv0;
    glm::vec2 deltaUV2 = uv2 - uv0;

    float det = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    if (std::abs(det) > 1e-6f) {
        float invDet = 1.0f / det;
        tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * invDet;
        bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * invDet;
    } else {
        tangent = edge1;
        bitangent = edge2;
    }

    tangent = glm::normalize(tangent - normal * glm::dot(normal, tangent));
    if (!std::isfinite(tangent.x) || !std::isfinite(tangent.y) || !std::isfinite(tangent.z)) {
        tangent = glm::normalize(edge1);
    }

    bitangent = glm::normalize(bitangent - normal * glm::dot(normal, bitangent));
    if (!std::isfinite(bitangent.x) || !std::isfinite(bitangent.y) || !std::isfinite(bitangent.z)) {
        bitangent = glm::normalize(glm::cross(normal, tangent));
    }

    if (glm::dot(glm::cross(tangent, bitangent), normal) < 0.0f) {
        bitangent = -bitangent;
    }

    vertices.push_back({p0, normal, glm::vec3(uv0, texIndex), tangent, bitangent, biome0});
    vertices.push_back({p1, normal, glm::vec3(uv1, texIndex), tangent, bitangent, biome1});
    vertices.push_back({p2, normal, glm::vec3(uv2, texIndex), tangent, bitangent, biome2});
    vertices.push_back({p3, normal, glm::vec3(uv3, texIndex), tangent, bitangent, biome3});

    indices.push_back(baseIdx);
    indices.push_back(baseIdx + 1);
    indices.push_back(baseIdx + 2);

    indices.push_back(baseIdx);
    indices.push_back(baseIdx + 2);
    indices.push_back(baseIdx + 3);
}

/* void ChunkMeshingSystem::generateMesh(Registry& registry, EntityID entity,
                                      SubChunkComponent& voxelData,
                                      MeshComponent& mesh,
                                      const WorldMapComponent& worldMap) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
} */

bool ChunkMeshingSystem::isMeshingComplete(Registry& registry) const {
    auto view = registry.view<SubChunkComponent>();
    
    for (EntityID entity : view) {
        if (registry.getComponent<SubChunkComponent>(entity).meshDirty) {
            return false;
        }
    }
    
    return true;
}

int ChunkMeshingSystem::getCompletedMeshCount(Registry& registry) const {
    auto view = registry.view<SubChunkComponent>();
    int completed = 0;
    
    for (EntityID entity : view) {
        if (!registry.getComponent<SubChunkComponent>(entity).meshDirty) {
            completed++;
        }
    }
    return completed;
}


MeshData ChunkMeshingSystem::calculateMeshData(Registry& registry, EntityID entity,
                                      SubChunkComponent& voxelData,
                                      const subChunkCache& cache) {
    MeshData result;
    int dims[3] = {SUBCHUNK_SIZE_X, SUBCHUNK_SIZE_Y, SUBCHUNK_SIZE_Z};

    glm::vec3 worldOffset(
        voxelData.subChunkPosition.x * 16.f,
        voxelData.subChunkPosition.y * 16.f,
        voxelData.subChunkPosition.z * 16.f
    );


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

                            VoxelType neighbor = getVoxelGlobal(voxelData, nx, ny, nz, cache);
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

                            float texIndex = static_cast<float>(BlockTextureManager::getTextureSliceIndex(type, axis, isPositive));

                            glm::vec3 calculatedNormal = glm::normalize(glm::cross(edge1, edge2));
                            if (glm::dot(calculatedNormal, normal) < 0) {
                                addFace(result.vertices, result.indices, corner, edge2, edge1, normal, texIndex, axis, isPositive, worldOffset, voxelData);
                            } else {
                                addFace(result.vertices, result.indices, corner, edge1, edge2, normal, texIndex, axis, isPositive, worldOffset, voxelData);
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
    return result; 
}


void ChunkMeshingSystem::uploadMeshToGPU(MeshComponent& mesh, const MeshData& data) {
    mesh.cleanup();
    if (data.vertices.empty()) {
        mesh.indexCount = 0;
        return;
    }

    glGenVertexArrays(1, &mesh.VAO);
    glBindVertexArray(mesh.VAO);

    glGenBuffers(1, &mesh.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(Vertex), data.vertices.data(), GL_STATIC_DRAW);

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
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, biomeColor));

    glGenBuffers(1, &mesh.IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(GLuint), data.indices.data(), GL_STATIC_DRAW);

    mesh.indexCount = data.indices.size();
    glBindVertexArray(0);
}

/* void ChunkMeshingSystem::update(Registry& registry, const WorldMapComponent& worldMap) {
    auto view = registry.view<SubChunkComponent, MeshComponent>();

    for (EntityID entity : view) {
        auto& voxelData = registry.getComponent<SubChunkComponent>(entity);

        // On ne met à jour le mesh QUE si le chunk a été modifié
        if (voxelData.meshDirty) {
            auto& mesh = registry.getComponent<MeshComponent>(entity);
            generateMesh(registry, entity, voxelData, mesh, worldMap);
            voxelData.meshDirty = false;
             */

void ChunkMeshingSystem::update(Registry& registry) {
    std::lock_guard<std::mutex> lock(uploadMutex);
    
    int uploadsThisFrame = 0;
    while (!uploadQueue.empty() && uploadsThisFrame < 150) {
        MeshResult result = uploadQueue.front();
        uploadQueue.pop();

        if (registry.hasComponent<MeshComponent>(result.entity)) {
            auto& mesh = registry.getComponent<MeshComponent>(result.entity);
            uploadMeshToGPU(mesh, result.data);
            
            if (registry.hasComponent<SubChunkComponent>(result.entity)) {
                registry.getComponent<SubChunkComponent>(result.entity).meshDirty = false;
            }
        }
        uploadsThisFrame++;
    }
}