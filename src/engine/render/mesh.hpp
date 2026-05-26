#ifndef MESH_HPP
#define MESH_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Mesh {
public:
    // === ATTRIBUTS ===
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;
    std::vector<unsigned int> indices;

    // === CONSTRUCTEUR ===
    Mesh();
    
    // === DESTRUCTEUR ===
    ~Mesh();

    // === GESTION DU CACHE DE MESHES ===
    static void clearMeshCache();
    
    // Récupérer ou créer le mesh du zombie en brut
    static std::shared_ptr<Mesh> getZombieMesh();

private:
    // === CACHE DE MESHES ===
    static std::unordered_map<std::string, std::shared_ptr<Mesh>> meshCache;
    
    // Helper pour ajouter une boîte texturée au mesh
    void addBox(const glm::vec3& center, const glm::vec3& size);
};

#endif
