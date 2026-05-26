#pragma once

#include "ecs/registry.hpp"
#include "ecs/components/mesh.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/chunk.hpp"
#include "ecs/components/lightingStateComponent.hpp"
#include <glm/glm.hpp>
#include <array>

class RenderSystem {
public:
    RenderSystem() = default;

    void update(Registry& registry, GLuint shaderProgram, const glm::vec3& lightColor, const glm::vec3& lightDirection);
    
    // Overload that reads lightColor and lightDirection from ECS
    void update(Registry& registry, GLuint shaderProgram);

private:
    void renderMesh(GLuint shaderProgram,
                    GLint locMVP,
                    const MeshComponent& mesh,
                    const glm::mat4& modelMatrix,
                    const glm::mat4& viewMatrix,
                    const glm::mat4& projectionMatrix,
                    const glm::vec3& lightColor,
                    const glm::vec3& lightDirection);
                    
    void extractFrustumPlanes(const glm::mat4& vpMatrix, std::array<glm::vec4, 6>& planes);
    bool isAABBInFrustum(const glm::vec3& minBounds, const glm::vec3& maxBounds, const std::array<glm::vec4, 6>& planes);
};
