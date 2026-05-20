#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

enum CameraMode {
    FIXED_CAMERA,      // Caméra fixe (vue isométrique)
    FREE_CAMERA,       // Caméra libre avec déplacement clavier
    ORBIT_CAMERA       // Caméra orbitale (pour future implémentation)
};

class Camera {
public:
    Camera();
    
    // Mise à jour de la caméra (inputs + calcul des matrices)
    void update(GLFWwindow* window, float deltaTime);
    
    // Getters pour les matrices
    glm::mat4 getViewMatrix() const { return viewMatrix; }
    glm::mat4 getProjectionMatrix() const { return projectionMatrix; }
    
    // Gestion des modes
    void setMode(CameraMode mode, GLFWwindow* window);
    CameraMode getMode() const { return currentMode; }
    
    // Initialisation avec position, cible et vecteur up
    void initialize(glm::vec3 position, glm::vec3 target, glm::vec3 up, float speed = 10.0f);
    
    // Paramètres de configuration
    void setSpeed(float speed) { freeSpeed = speed; }
    void setMouseSpeed(float speed) { freeMouseSpeed = speed; }
    void setFOV(float fov) { fieldOfView = fov; }
    void setOrbitSpeed(float speed);
    float getOrbitSpeed() const { return orbitSpeed; }
    
private:
    // Matrices
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    
    // Mode actuel
    CameraMode currentMode;
    
    // Paramètres de projection
    float fieldOfView;
    
    // Caméra fixe
    glm::vec3 fixedPosition;
    glm::vec3 fixedTarget;
    glm::vec3 fixedUp;
    
    // Caméra libre
    glm::vec3 freePosition;
    float freeHorizontalAngle;
    float freeVerticalAngle;
    float freeSpeed;
    float freeMouseSpeed;

    // Caméra orbitale
    glm::vec3 orbitCenter;
    float orbitRadius;
    float orbitAngle;
    float orbitSpeed;
    float orbitHeightOffset;
    
    // Méthodes privées pour chaque mode
    void updateFixedCamera();
    void updateFreeCamera(GLFWwindow* window, float deltaTime);
    void updateOrbitCamera(GLFWwindow* window, float deltaTime);

    // Fonction privée pour calculer le ratio d'aspect
    float computeAspect(GLFWwindow* window) const;
};

#endif
