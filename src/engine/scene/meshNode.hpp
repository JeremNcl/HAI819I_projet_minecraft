#ifndef MESHNODE_HPP
#define MESHNODE_HPP

#include "sceneNode.hpp"
#include "../render/mesh.hpp"

#include <GL/glew.h>

#include <memory>
#include <string>


/**
 * Nœud de scène avec géométrie (mesh)
 * Peut être rendu à l'écran
 */
class MeshNode : public SceneNode {
public:
    // === CONSTRUCTEUR ===
    MeshNode(const std::string& name, std::shared_ptr<Mesh> mesh = nullptr);
    
    // === DESTRUCTEUR ===
    virtual ~MeshNode();
    
    // === SETTERS ===
    void setMesh(std::shared_ptr<Mesh> mesh) { this->mesh = mesh; }
    void setShaderProgram(GLuint shaderProgram) { this->shaderProgram = shaderProgram; }
    void setTexture(GLuint texture) { this->texture = texture; }

    // === GETTERS ===
    std::shared_ptr<Mesh> getMesh() const { return mesh; }
    GLuint getShaderProgram() const { return shaderProgram; }
    GLuint getTexture() const { return texture; }

    // === GESTION DES BUFFERS ET TEXTURES ===
    bool hasBindedBuffers() const;
    void bindBuffers();
    void deleteBuffers();
    virtual void bindTextures();

    // === RENDU ===
    virtual void draw(const glm::mat4& viewProjection) override;
    
    // === GESTION DU CACHE DE MESHES ===
    static void clearMeshCache();
    
protected:
    // Mesh
    std::shared_ptr<Mesh> mesh;
    // Buffers
    GLuint vertexBuffer;
    GLuint indexBuffer;
    GLuint uvBuffer;
    GLuint normalBuffer;
    size_t indexCount;
    // Texture et shader
    GLuint texture;
    GLuint shaderProgram;
};

#endif