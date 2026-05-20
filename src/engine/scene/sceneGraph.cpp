#include "sceneGraph.hpp"


SceneGraph::SceneGraph() {
    // Créer le nœud racine
    root = std::make_shared<SceneNode>("Root");
}

SceneGraph::~SceneGraph() {
    // Le nœud racine et tous ses enfants seront détruits automatiquement grâce aux shared_ptr
}

void SceneGraph::update(float deltaTime) {
    if (root) {
        root->update(deltaTime);
    }
}

void SceneGraph::draw(const glm::mat4& viewProjection) {
    if (root) {
        root->draw(viewProjection);
    }
}

SceneNode* SceneGraph::findNode(const std::string& name) {
    if (!root) return nullptr;
    
    // Vérifier d'abord la racine
    if (root->getName() == name) {
        return root.get();
    }
    
    // Sinon chercher dans les enfants
    return findNodeRecursive(root.get(), name);
}

SceneNode* SceneGraph::findNodeRecursive(SceneNode* node, const std::string& name) {
    if (!node) return nullptr;
    
    // Chercher dans tous les enfants
    for (const auto& child : node->getChildren()) {
        if (!child) continue;
        
        // Si c'est le bon nœud
        if (child->getName() == name) {
            return child.get();
        }
        
        // Sinon chercher récursivement dans ses enfants
        SceneNode* result = findNodeRecursive(child.get(), name);
        if (result) {
            return result;
        }
    }
    
    return nullptr;
}

int SceneGraph::getNodeCount() const {
    if (!root) return 0;
    return countNodesRecursive(root.get());
}

int SceneGraph::countNodesRecursive(SceneNode* node) const {
    if (!node) return 0;
    
    int count = 1; // Compter le nœud actuel
    
    // Ajouter le compte de tous les enfants
    for (const auto& child : node->getChildren()) {
        count += countNodesRecursive(child.get());
    }
    
    return count;
}
