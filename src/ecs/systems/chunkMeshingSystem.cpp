#include "chunkMeshingSystem.hpp"
#include <algorithm>
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


void ChunkMeshingSystem::addFace(std::vector<Vertex>& vertices,
                                  std::vector<GLuint>& indices,
                                  glm::vec3 corner,
                                  glm::vec3 edge1,
                                  glm::vec3 edge2,
                                  glm::vec3 normal,
                                  float texIndex,
                                  int axis,
                                  glm::vec3 worldOffset) {
    GLuint baseIdx = static_cast<GLuint>(vertices.size());

    auto getUV = [axis](glm::vec3 localPos) -> glm::vec2 {
        if (axis == 0) return glm::vec2(localPos.z, localPos.y);
        if (axis == 1) return glm::vec2(localPos.x, localPos.z);
        return glm::vec2(localPos.x, localPos.y);
    };

    glm::vec3 local_p0 = corner;
    glm::vec3 local_p1 = corner + edge1;
    glm::vec3 local_p2 = corner + edge1 + edge2;
    glm::vec3 local_p3 = corner + edge2;

    glm::vec3 p0 = local_p0 + worldOffset;
    glm::vec3 p1 = local_p1 + worldOffset;
    glm::vec3 p2 = local_p2 + worldOffset;
    glm::vec3 p3 = local_p3 + worldOffset;

    vertices.push_back({p0, normal, glm::vec3(getUV(local_p0), texIndex)});
    vertices.push_back({p1, normal, glm::vec3(getUV(local_p1), texIndex)});
    vertices.push_back({p2, normal, glm::vec3(getUV(local_p2), texIndex)});
    vertices.push_back({p3, normal, glm::vec3(getUV(local_p3), texIndex)});

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
                                addFace(result.vertices, result.indices, corner, edge2, edge1, normal, texIndex, axis, worldOffset);
                            } else {
                                addFace(result.vertices, result.indices, corner, edge1, edge2, normal, texIndex, axis, worldOffset);
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

void ChunkMeshingSystem::update(Registry& registry, const WorldMapComponent& worldMap) {
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