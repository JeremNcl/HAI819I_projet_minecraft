#include "mesh.hpp"

#include <cmath>
#include <iostream>

#include <glm/gtc/constants.hpp>


// === CACHE DE MESHES ===
std::unordered_map<std::string, std::shared_ptr<Mesh>> Mesh::meshCache;


// === CONSTRUCTEURS ===

Mesh::Mesh(){}


// === DESTRUCTEUR ===

Mesh::~Mesh(){}


// === GESTION DU CACHE DE MESHES ===

void Mesh::clearMeshCache() {
    std::cout << "Vidage du cache de meshes (" << meshCache.size() << " entrées)" << std::endl;
    meshCache.clear();
}