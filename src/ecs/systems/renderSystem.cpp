#include "renderSystem.hpp"
#include "ecs/components/camera.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <unordered_map>
#include <unordered_set>
#include <queue>


#ifndef GLM_VEC3_HASH_DEFINED
#define GLM_VEC3_HASH_DEFINED
struct GLMVec3Hash {
    std::size_t operator()(const glm::ivec3& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1) ^ (std::hash<int>()(k.z) << 2);
    }
};
#endif


void RenderSystem::extractFrustumPlanes(const glm::mat4& vp, std::array<glm::vec4, 6>& planes) {
    planes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);
    planes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);
    planes[2] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);
    planes[3] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);
    planes[4] = glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]);
    planes[5] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

    for (int i = 0; i < 6; i++) {
        float length = glm::length(glm::vec3(planes[i]));
        planes[i] /= length;
    }
}

bool RenderSystem::isAABBInFrustum(const glm::vec3& min, const glm::vec3& max, const std::array<glm::vec4, 6>& planes) {
    glm::vec3 center = (min + max) * 0.5f;
    glm::vec3 extents = (max - min) * 0.5f;

    for (int i = 0; i < 6; i++) {
        glm::vec3 normal = glm::vec3(planes[i]);
        
        float r = extents.x * std::abs(normal.x) + 
                  extents.y * std::abs(normal.y) + 
                  extents.z * std::abs(normal.z);
        
        float d = glm::dot(normal, center) + planes[i].w;

        if (d < -r) {
            return false; 
        }
    }
    return true;
}

void RenderSystem::update(Registry& registry, GLuint shaderProgram, const glm::vec3& lightColor) {    
    auto view = registry.view<MeshComponent, TransformComponent, SubChunkComponent>();
    Registry::View cameraView = registry.view<CameraComponent>();

    if (cameraView.isEmpty()) return;
    
    static EntityID lastActiveCamera = -1;
    CameraComponent* camera = nullptr;

    if (lastActiveCamera != -1 && registry.hasComponent<CameraComponent>(lastActiveCamera)) {
        CameraComponent& cam = registry.getComponent<CameraComponent>(lastActiveCamera);
        if (cam.isActive) camera = &cam;
    }

    if (!camera) {
        for (EntityID cameraEntity : cameraView) {
            CameraComponent& testedCamera = registry.getComponent<CameraComponent>(cameraEntity);
            if (testedCamera.isActive){
                camera = &testedCamera;
                lastActiveCamera = cameraEntity;
                break;
            }
        }
    }

    if (!camera) return;

    glUseProgram(shaderProgram);

    if (programUniformsCache.find(shaderProgram) == programUniformsCache.end()) {
        programUniformsCache[shaderProgram] = cacheUniformLocations(shaderProgram);
    }
    
    const ShaderUniforms& uniforms = programUniformsCache[shaderProgram];


    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 vpMatrix = camera->projectionMatrix * camera->viewMatrix;
    glm::mat3 normalMatrix = glm::mat3(1.0f);

    glUniformMatrix4fv(uniforms.mvp, 1, GL_FALSE, glm::value_ptr(vpMatrix));
    glUniformMatrix4fv(uniforms.model, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(uniforms.view, 1, GL_FALSE, glm::value_ptr(camera->viewMatrix));
    glUniformMatrix4fv(uniforms.projection, 1, GL_FALSE, glm::value_ptr(camera->projectionMatrix));
    glUniformMatrix3fv(uniforms.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    glm::vec3 viewPos = glm::vec3(glm::inverse(camera->viewMatrix)[3]);
    glUniform3fv(uniforms.viewPos, 1, glm::value_ptr(viewPos));

    glm::vec3 lightPos(128.0f, 400.0f, 128.0f);
    glUniform3fv(uniforms.lightPos, 1, glm::value_ptr(lightPos));
    glUniform3fv(uniforms.lightColor, 1, glm::value_ptr(lightColor));


    std::array<glm::vec4, 6> frustumPlanes;
    extractFrustumPlanes(vpMatrix, frustumPlanes);

    std::unordered_map<glm::ivec3, EntityID, GLMVec3Hash> grid;
    for (EntityID entity : view) {
        const auto& subChunk = registry.getComponent<SubChunkComponent>(entity);
        grid[subChunk.subChunkPosition] = entity;
    }

    struct BFSNode {
        glm::ivec3 pos;
        int entryFace;
    };

    std::queue<BFSNode> queue;
    std::unordered_set<glm::ivec3, GLMVec3Hash> visited;

    int camChunkX = static_cast<int>(std::floor(viewPos.x / 16.0f));
    int camSubY   = static_cast<int>(std::floor(viewPos.y / 16.0f));
    int camChunkZ = static_cast<int>(std::floor(viewPos.z / 16.0f));
    glm::ivec3 startPos(camChunkX, camSubY, camChunkZ);

    queue.push({startPos, -1});
    visited.insert(startPos);

    const glm::ivec3 DIRS[6] = {
        {-1, 0, 0}, {1, 0, 0},
        {0, -1, 0}, {0, 1, 0},
        {0, 0, -1}, {0, 0, 1}
    };
    const int OPPOSITE_FACE[6] = {1, 0, 3, 2, 5, 4};

    /* struct RenderNode {
        EntityID entity;
        float distanceSq;
        const MeshComponent* mesh;
    };

    std::vector<RenderNode> visibleChunks;
    visibleChunks.reserve(2000);

    glm::vec3 camPos = registry.getComponent<TransformComponent>(lastActiveCamera).position;

    for (EntityID entity : view) {
        const auto& mesh = registry.getComponent<MeshComponent>(entity);
        if (mesh.indexCount == 0 || mesh.VAO == 0) continue;

        const auto& transform = registry.getComponent<TransformComponent>(entity);
        const auto& subChunk = registry.getComponent<SubChunkComponent>(entity);
        
        if (!subChunk.visibility.isConnected())

        glm::vec3 minBounds = (glm::vec3(subChunk.subChunkPosition) * 16.0f) + transform.position;
        glm::vec3 maxBounds = minBounds + glm::vec3(16.0f, 16.0f, 16.0f);
        
        if (!isAABBInFrustum(minBounds, maxBounds, frustumPlanes)) continue;

        glm::vec3 diff = camPos - ((minBounds + maxBounds) * 0.5f);
        visibleChunks.push_back({entity, glm::dot(diff, diff), &mesh});
    }

    std::sort(visibleChunks.begin(), visibleChunks.end(), [](const RenderNode& a, const RenderNode& b) {
        return a.distanceSq < b.distanceSq; 
    });

    // 3. RENDU BATCHÉ
    for (const auto& node : visibleChunks) {
        glBindVertexArray(node.mesh->VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(node.mesh->indexCount), GL_UNSIGNED_INT, nullptr);
    }
    
    glBindVertexArray(0);
    glUseProgram(0); */

    struct RenderNode { float distanceSq; const MeshComponent* mesh; };
    std::vector<RenderNode> visibleChunks;
    visibleChunks.reserve(1000);

    // 4. Boucle principale de propagation
    while (!queue.empty()) {
        BFSNode current = queue.front();
        queue.pop();

        auto it = grid.find(current.pos);
        if (it == grid.end()) continue; // Le chunk n'est pas chargé / pas prêt

        EntityID entity = it->second;
        const auto& subChunk = registry.getComponent<SubChunkComponent>(entity);
        const auto& mesh = registry.getComponent<MeshComponent>(entity);
        const auto& transform = registry.getComponent<TransformComponent>(entity);

        if (current.entryFace == -1 && subChunk.solidBlockCount == 4096) {
            continue; 
        }

        // Test Frustum Culling sur l'AABB du sous-chunk (16x16x16)
        glm::vec3 minBounds = (glm::vec3(current.pos) * 16.0f) + transform.position;
        glm::vec3 maxBounds = minBounds + glm::vec3(16.0f, 16.0f, 16.0f);
        
        if (!isAABBInFrustum(minBounds, maxBounds, frustumPlanes)) {
            // Si on est en dehors du champ de vision, on n'explore pas les voisins cachés derrière
            if (current.pos != startPos) continue; 
        }

        // Si le sous-chunk a de la géométrie, on l'ajoute à la liste finale de rendu
        if (mesh.indexCount > 0 && mesh.VAO != 0) {
            glm::vec3 diff = viewPos - ((minBounds + maxBounds) * 0.5f);
            visibleChunks.push_back({glm::dot(diff, diff), &mesh});
        }

        // On tente de se propager vers les 6 sous-chunks voisins
        for (int i = 0; i < 6; ++i) {
            
            if (current.entryFace != -1 && !subChunk.visibility.isConnected(current.entryFace, i)) {
                continue; // Bloqué par la roche ! On "cull" toute cette branche de l'arbre de rendu.
            }

            glm::ivec3 neighborPos = current.pos + DIRS[i];

            // Si le voisin n'a pas encore été visité dans cette frame
            if (visited.find(neighborPos) == visited.end()) {
                if (neighborPos.y >= 0 && neighborPos.y < 16) { // Reste dans les limites de hauteur du monde
                    visited.insert(neighborPos);
                    // On entre chez le voisin par la face opposée (ex: si je sors par +X, j'entre chez lui par -X)
                    queue.push({neighborPos, OPPOSITE_FACE[i]});
                }
            }
        }
    }

    // 5. Tri des sommets (Front-to-Back) pour l'Early-Z du GPU
    std::sort(visibleChunks.begin(), visibleChunks.end(), [](const RenderNode& a, const RenderNode& b) {
        return a.distanceSq < b.distanceSq; 
    });

    // 6. Rendu final des maillages validés
    for (const auto& node : visibleChunks) {
        glBindVertexArray(node.mesh->VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(node.mesh->indexCount), GL_UNSIGNED_INT, nullptr);
    }
    
    glBindVertexArray(0);
    glUseProgram(0);
}
