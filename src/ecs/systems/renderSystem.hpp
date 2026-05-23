#pragma once

#include "../registry.hpp"
#include "../components/mesh.hpp"
#include "../components/transform.hpp"
#include "../components/chunk.hpp"
#include <glm/glm.hpp>
#include <array>

class RenderSystem {
public:
    RenderSystem() = default;

    void update(Registry& registry, GLuint shaderProgram);

private:
    void renderMesh(GLuint shaderProgram,
                    GLint locMVP,
                    const MeshComponent& mesh,
                    const glm::mat4& modelMatrix,
                    const glm::mat4& viewMatrix,
                    const glm::mat4& projectionMatrix);
                    
    void extractFrustumPlanes(const glm::mat4& vpMatrix, std::array<glm::vec4, 6>& planes);
    bool isAABBInFrustum(const glm::vec3& minBounds, const glm::vec3& maxBounds, const std::array<glm::vec4, 6>& planes);
};
