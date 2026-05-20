#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <vector>
#include <memory>


class Transform : public std::enable_shared_from_this<Transform> {

    // === Membres privés : état interne et hiérarchie ===
private:
    glm::vec3 translationVec{0.f}; // translation vector
    glm::mat4 rotationMat{1.f};    // rotation matrix
    glm::vec3 scaleVec{1.f};       // uniform scale

    std::weak_ptr<Transform> parent;
    std::vector<std::shared_ptr<Transform>> children;
    
    mutable glm::mat4 cachedWorldMatrix{1.0f};
    mutable bool worldMatrixDirty{true};

    // === API publique : interface, gestion hiérarchie, transformations ===
public: 
    // === Constructeurs et destructeur ===
    Transform(std::weak_ptr<Transform> parent = {});
    Transform(const glm::vec3& _s, const glm::mat4& _r, const glm::vec3& _t, std::weak_ptr<Transform> _parent = {});
    Transform(const Transform& _transform, std::weak_ptr<Transform> _parent = {});
    ~Transform();

    // === Getters ===   
    glm::vec3 getLocalTranslation() const { return translationVec; }
    glm::mat4 getLocalRotation() const { return rotationMat; }
    glm::vec3 getLocalScale() const { return scaleVec; }

    glm::mat4 getLocalMatrix() const;
    glm::mat4 getWorldMatrix() const;

    std::shared_ptr<Transform> getParent() const { return parent.lock(); }
    std::vector<std::shared_ptr<Transform>> getAllChildren() const { return children; }

    // === Setters ===
    void setTranslation(const glm::vec3& _t);
    void setRotation(const glm::mat4& _r);
    void setRotation(const glm::quat& q);
    void setRotation(const glm::vec3& eulerAngles);
    void setScale(const glm::vec3& _s);
    
    // === Gestion hiérarchie ===
    void addChild(const std::shared_ptr<Transform>& _child);
    void removeChild(const std::shared_ptr<Transform>& _child);
    void removeAllChildren();
    void setParent(const std::shared_ptr<Transform>& _parent);

    void markWorldMatrixDirty();

    // === Utilitaires ===
    bool isAncestorOf(const Transform* _other) const;

    // === Transformations locales ===
    void translate(const glm::vec3& _t);
    void rotate(const glm::mat4& _r);
    void rotate(const glm::quat& q); // surcharge quaternion
    void rotate(const glm::vec3& eulerAngles); // surcharge angles d'Euler (radians)
    void scale(const glm::vec3& _s);
};

#endif