#ifndef SCENEGRAPH_HPP
#define SCENEGRAPH_HPP

#include "sceneNode.hpp"

#include <memory>

#include <glm/glm.hpp>

/**
 * Gestionnaire principal du graphe de scène
 * Contient le nœud racine et gère la mise à jour/rendu de toute la scène
 */
class SceneGraph {
public:
    SceneGraph();
    ~SceneGraph();
    
    // Accès au nœud racine
    std::shared_ptr<SceneNode> getRoot() { return root; }
    const std::shared_ptr<SceneNode> getRoot() const { return root; }
    
    // Mise à jour de toute la scène (récursif depuis la racine)
    void update(float deltaTime);
    
    // Rendu de toute la scène (récursif depuis la racine)
    void draw(const glm::mat4& viewProjection);
    
    // Recherche d'un nœud par son nom (utile pour le debug)
    SceneNode* findNode(const std::string& name);
    
    // Statistiques (optionnel, pour le debug)
    int getNodeCount() const;
    
private:
    std::shared_ptr<SceneNode> root;
    
    // Méthodes privées pour la recherche récursive
    SceneNode* findNodeRecursive(SceneNode* node, const std::string& name);
    int countNodesRecursive(SceneNode* node) const;
};

#endif
