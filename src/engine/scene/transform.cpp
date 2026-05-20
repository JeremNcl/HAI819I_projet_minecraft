#include "transform.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <algorithm>
#include <memory>


// === Constructeurs et destructeur ===
Transform::Transform(std::weak_ptr<Transform> _parent): 
    scaleVec(glm::vec3(1.0f)),
    rotationMat(glm::mat4(1.0f)),
    translationVec(glm::vec3(0.0f)),
    parent(_parent)
{}

Transform::Transform(const glm::vec3& _s, const glm::mat4& _r, const glm::vec3& _t, std::weak_ptr<Transform> _parent):
    scaleVec(_s),
    rotationMat(_r),
    translationVec(_t),
    parent(_parent)
{}

Transform::Transform(const Transform& _transform, std::weak_ptr<Transform> _parent):
    scaleVec(_transform.scaleVec),
    rotationMat(_transform.rotationMat),
    translationVec(_transform.translationVec),
    parent(_parent)
{}

Transform::~Transform() {
    auto p = parent.lock();
    if(p) {
        auto& pChildren = p->children;
        pChildren.erase(std::remove_if(pChildren.begin(), pChildren.end(), [this](const std::shared_ptr<Transform>& c){ return c.get() == this; }), pChildren.end());
    }
    for (auto& child : children) {
        if (child) {
            child->parent.reset();
        }
    }
    children.clear();
}


// === Getters ===
glm::mat4 Transform::getLocalMatrix() const {
    glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), translationVec);
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scaleVec);
    return translationMat * rotationMat * scaleMat;
}

glm::mat4 Transform::getWorldMatrix() const {
    if (!worldMatrixDirty) return cachedWorldMatrix;
    
    glm::mat4 localMatrix = getLocalMatrix();

    auto p = parent.lock();
    if (p) {
        cachedWorldMatrix = p->getWorldMatrix() * localMatrix;
    } else {
        cachedWorldMatrix = localMatrix;
    }

    worldMatrixDirty = false;
    return cachedWorldMatrix;
}

// === Gestion hiérarchie ===
void Transform::addChild(const std::shared_ptr<Transform>& _child)
{
    if (!_child || _child.get() == this) return;
    if (isAncestorOf(_child.get())) return;
    for (auto& c : children) {
        if (c == _child) return;
    }
    children.push_back(_child);
    _child->setParent(shared_from_this());
    markWorldMatrixDirty();
}

void Transform::removeChild(const std::shared_ptr<Transform>& _child){
    auto it = std::remove(children.begin(), children.end(), _child);
    if (it != children.end()) {
        children.erase(it, children.end());
        if (_child->parent.lock().get() == this) _child->parent.reset();
    }
    markWorldMatrixDirty();
}

void Transform::removeAllChildren() {
    for (auto& child : children) {
        if (child && child->parent.lock().get() == this) {
            child->parent.reset();
        }
    }
    children.clear();
    markWorldMatrixDirty();
}

void Transform::setParent(const std::shared_ptr<Transform>& _parent)
{
    if (_parent.get() == this || (_parent && _parent->isAncestorOf(this))) return;
    auto p = parent.lock();
    if (p) p->removeChild(shared_from_this());
    parent = _parent;
    markWorldMatrixDirty();
}

void Transform::markWorldMatrixDirty() {
    worldMatrixDirty = true;
    for (auto& child : children) {
        if (child) child->markWorldMatrixDirty();
    }
}

// === Setters ===
void Transform::setScale(const glm::vec3& _s) { 
    scaleVec = _s;
    markWorldMatrixDirty();
}

void Transform::setRotation(const glm::mat4& _r) {
    rotationMat = _r;
    markWorldMatrixDirty();
}

void Transform::setRotation(const glm::quat& q) {
    rotationMat = glm::mat4_cast(q);
    markWorldMatrixDirty();
}

void Transform::setRotation(const glm::vec3& eulerAngles) {
    // Convention classique (x=pitch, y=yaw, z=roll)
    rotationMat = glm::mat4(glm::yawPitchRoll(eulerAngles.y, eulerAngles.x, eulerAngles.z));
    markWorldMatrixDirty();
}

void Transform::setTranslation(const glm::vec3& _t) {
    translationVec = _t; 
    markWorldMatrixDirty();
}

// === Utilitaires ===
bool Transform::isAncestorOf(const Transform* _other) const {
    if (!_other) return false;
    auto current = _other->parent.lock();
    while (current) {
        if (current.get() == this) return true;
        current = current->parent.lock();
    }
    return false;
}

// === Transformations locales ===
void Transform::scale(const glm::vec3& _s) {
    scaleVec *= _s;
    markWorldMatrixDirty();
}

void Transform::rotate(const glm::mat4& _r) {
    rotationMat = _r * rotationMat;
    markWorldMatrixDirty();
}

void Transform::rotate(const glm::quat& q) {
    rotationMat = glm::mat4_cast(q) * rotationMat;
    markWorldMatrixDirty();
}

void Transform::rotate(const glm::vec3& eulerAngles) {
    // Convention classique (x=pitch, y=yaw, z=roll)
    glm::mat4 rot = glm::mat4(glm::yawPitchRoll(eulerAngles.y, eulerAngles.x, eulerAngles.z));
    rotationMat = rot * rotationMat;
    markWorldMatrixDirty();
}

void Transform::translate(const glm::vec3& _t) {
    translationVec += _t;
    markWorldMatrixDirty();
}
