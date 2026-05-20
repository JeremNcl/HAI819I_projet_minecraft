#include "sceneNode.hpp"
#include <algorithm>

SceneNode::SceneNode(const std::string& name)
    : name(name)
    , isActive(true)
    , parent(nullptr)
    , transform(std::make_shared<Transform>())
{}

SceneNode::~SceneNode() {
    // Détacher tous les enfants
    for (auto& child : children) {
        if (child) {
            child->parent = nullptr;
        }
    }
}

void SceneNode::addChild(std::shared_ptr<SceneNode> child) {
    if (!child) return;

    // Protection : ne pas ajouter soi-même
    if (child.get() == this) return;

    // Protection : ne pas créer de cycle
    if (child->isAncestorOf(this)) return;

    // Retirer l'enfant de son ancien parent
    if (child->parent) {
        child->parent->removeChild(child);
    }

    // Ajouter comme enfant
    children.push_back(child);
    child->parent = this;

    // Mettre à jour la hiérarchie des transforms
    transform->addChild(child->transform);
}

void SceneNode::removeChild(std::shared_ptr<SceneNode> child) {
    if (!child) return;

    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        // Symétrie : on détache le parent côté enfant
        (*it)->parent = nullptr;
        (*it)->transform->setParent(nullptr);

        children.erase(it);
    }
}


bool SceneNode::isAncestorOf(const SceneNode* node) const {
    if (!node) return false;
    const SceneNode* current = node->parent;
    while (current) {
        if (current == this) return true;
        current = current->parent;
    }
    return false;
}

void SceneNode::update(float deltaTime) {
    if (!isActive) return;
    
    // Mettre à jour tous les enfants récursivement
    for (auto& child : children) {
        if (child && child->getActive()) {
            child->update(deltaTime);
        }
    }
}

void SceneNode::draw(const glm::mat4& viewProjection) {
    if (!isActive) return;
    
    // Dessiner tous les enfants récursivement
    for (auto& child : children) {
        if (child && child->getActive()) {
            child->draw(viewProjection);
        }
    }
}
