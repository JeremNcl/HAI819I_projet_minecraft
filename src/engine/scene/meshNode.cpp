#include "meshNode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <iostream>


// === CONSTRUCTEURS ===

MeshNode::MeshNode(const std::string& name, std::shared_ptr<Mesh> mesh):
    SceneNode(name),
    mesh(mesh), 
    vertexBuffer(0),
    indexBuffer(0),
    uvBuffer(0),
    normalBuffer(0),
    indexCount(0),
    texture(0),
    shaderProgram(0)
{}


// === DESTRUCTEUR ===

MeshNode::~MeshNode() {
    deleteBuffers();
}


// === GESTION DES BUFFERS ET TEXTURES ===

bool MeshNode::hasBindedBuffers() const {
    return vertexBuffer != 0 && indexBuffer != 0 && indexCount > 0;
}

void MeshNode::bindBuffers()
{
    if (!mesh || mesh->vertices.empty() || mesh->indices.empty()) {
        return;
    }

    if (vertexBuffer == 0) {
        glGenBuffers(1, &vertexBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertices.size() * sizeof(glm::vec3), mesh->vertices.data(), GL_STATIC_DRAW);

    if (indexBuffer == 0) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(unsigned int), mesh->indices.data(), GL_STATIC_DRAW);

    if (!mesh->uvs.empty()) {
        if (uvBuffer == 0) {
            glGenBuffers(1, &uvBuffer);
        }
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glBufferData(GL_ARRAY_BUFFER, mesh->uvs.size() * sizeof(glm::vec2), mesh->uvs.data(), GL_STATIC_DRAW);
    }

    if (!mesh->normals.empty()) {
        if (normalBuffer == 0) {
            glGenBuffers(1, &normalBuffer);
        }
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        glBufferData(GL_ARRAY_BUFFER, mesh->normals.size() * sizeof(glm::vec3), mesh->normals.data(), GL_STATIC_DRAW);
    }

    indexCount = mesh->indices.size();
}

void MeshNode::deleteBuffers() {
    
    if (vertexBuffer != 0) {
        glDeleteBuffers(1, &vertexBuffer);
        vertexBuffer = 0;
    }
    if (indexBuffer != 0) {
        glDeleteBuffers(1, &indexBuffer);
        indexBuffer = 0;
    }
    if (uvBuffer != 0) {
        glDeleteBuffers(1, &uvBuffer);
        uvBuffer = 0;
    }
    if (normalBuffer != 0) {
        glDeleteBuffers(1, &normalBuffer);
        normalBuffer = 0;
    }
}

void MeshNode::bindTextures() {
    if (texture != 0) {
        // Indique au shader qu'il y a une texture
        GLuint hasTextureID = glGetUniformLocation(shaderProgram, "hasTexture");
        if (hasTextureID != (GLuint)-1) {
            glUniform1i(hasTextureID, 1);
        }
        // Binder la texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        GLuint textureID = glGetUniformLocation(shaderProgram, "textureSampler");
        if (textureID != (GLuint)-1) {
            glUniform1i(textureID, 0);
        }
    } else {
        // Indique au shader qu'il n'y a pas de texture
        GLuint hasTextureID = glGetUniformLocation(shaderProgram, "hasTexture");
        if (hasTextureID != (GLuint)-1) {
            glUniform1i(hasTextureID, 0);
        }
    }
}


// === RENDU ===

void MeshNode::draw(const glm::mat4& viewProjection) {
    
    // Vérification du mesh
    if (!isActive || !mesh || mesh->indices.size() == 0) {
        // Dessiner quand même les enfants
        SceneNode::draw(viewProjection);
        return;
    }
    // Vérification des buffers
    if (!hasBindedBuffers()) {
        bindBuffers();
    }
    // Vérification du shader
    if (shaderProgram == 0) {
        SceneNode::draw(viewProjection);
        return;
    }
    
    // Utiliser le shader du mesh
    glUseProgram(shaderProgram);
    
    // Calcul des matrices
    glm::mat4 model = transform->getWorldMatrix();
    glm::mat4 MVP = viewProjection * model;
    
    // Envoi de la matrice MVP au shader
    GLuint mvpID = glGetUniformLocation(shaderProgram, "MVP");
    if (mvpID != (GLuint)-1) {
        glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
    }
    
    // Envoi de la matrice Model au shader
    GLuint modelID = glGetUniformLocation(shaderProgram, "M");
    if (modelID != (GLuint)-1) {
        glUniformMatrix4fv(modelID, 1, GL_FALSE, &model[0][0]);
    }
    
    // Binder les textures
    bindTextures();
    
    // Vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    // UVs (si présents)
    if (uvBuffer != 0) {
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    }
    
    // Normals (si présents)
    if (normalBuffer != 0) {
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    }
    
    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    
    // Dessiner
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)0);
    
    // Désactiver les attributs
    glDisableVertexAttribArray(0);
    if (uvBuffer != 0) glDisableVertexAttribArray(1);
    if (normalBuffer != 0) glDisableVertexAttribArray(2);
    
    // Dessiner les enfants
    SceneNode::draw(viewProjection);
}


// === GESTION DU CACHE DE MESHES ===

void MeshNode::clearMeshCache() {
    Mesh::clearMeshCache();
}