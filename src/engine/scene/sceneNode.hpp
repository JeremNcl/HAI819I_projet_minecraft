#ifndef SCENENODE_HPP
#define SCENENODE_HPP

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "transform.hpp"

/**
 * Classe de base pour tous les nœuds du graphe de scène
 * Gère la hiérarchie parent-enfants et contient un Transform
 */
class SceneNode {
public:
    SceneNode(const std::string& name = "Node");
    virtual ~SceneNode();

    // === Accès au Transform ===
    Transform& getTransform() { return *transform; }
    const Transform& getTransform() const { return *transform; }

    // === Gestion de la hiérarchie ===
    void addChild(std::shared_ptr<SceneNode> child);
    void removeChild(std::shared_ptr<SceneNode> child);
    SceneNode* getParent() const { return parent; }
    const std::vector<std::shared_ptr<SceneNode>>& getChildren() const { return children; }

    // === Utilitaire pour vérifier si ce node est ancêtre d'un autre ===
    bool isAncestorOf(const SceneNode* node) const;

    // === Mise à jour et rendu (virtuels pour permettre la surcharge) ===
    virtual void update(float deltaTime);
    virtual void draw(const glm::mat4& viewProjection);

    // === Activation/Désactivation ===
    void setActive(bool active) { isActive = active; }
    bool getActive() const { return isActive; }

    // === Nom (utile pour le debug et la recherche) ===
    std::string getName() const { return name; }
    void setName(const std::string& n) { name = n; }

protected:
    std::string name;
    bool isActive;

    // Transform encapsulé
    std::shared_ptr<Transform> transform;

    // Hiérarchie
    SceneNode* parent;
    std::vector<std::shared_ptr<SceneNode>> children;
};

#endif
