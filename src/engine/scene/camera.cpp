// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

#include "camera.hpp"

Camera::Camera() 
    : currentMode(FIXED_CAMERA),
      fieldOfView(45.0f),
      freeHorizontalAngle(3.14f),
      freeVerticalAngle(-0.3f),
      freeSpeed(10.0f),
      freeMouseSpeed(0.002f),
      fixedPosition(glm::vec3(0, 0, 5)),
      fixedTarget(glm::vec3(0, 0, 0)),
      fixedUp(glm::vec3(0, 1, 0)),
    freePosition(glm::vec3(0, 10, 20)),
    orbitCenter(glm::vec3(0, 0, 0)),
    orbitRadius(5.0f),
    orbitAngle(0.0f),
    orbitSpeed(0.5f),
    orbitHeightOffset(0.0f)
{
    // Initialiser les matrices avec des valeurs par défaut
    projectionMatrix = glm::perspective(glm::radians(fieldOfView), 4.0f / 3.0f, 0.1f, 5000.0f);
    viewMatrix = glm::lookAt(fixedPosition, fixedTarget, fixedUp);
}

// Fonction privée pour calculer le ratio d'aspect
float Camera::computeAspect(GLFWwindow* window) const {
    int width = 1024, height = 768;
    if (window) {
        glfwGetFramebufferSize(window, &width, &height);
    } else {
        GLFWwindow* ctx = glfwGetCurrentContext();
        if (ctx) glfwGetFramebufferSize(ctx, &width, &height);
    }
    return (height > 0) ? (float)width / (float)height : 4.0f / 3.0f;
}

void Camera::initialize(glm::vec3 position, glm::vec3 target, glm::vec3 up, float speed) {
    // Initialiser la caméra fixe
    fixedPosition = position;
    fixedTarget = target;
    fixedUp = up;
    
    // Initialiser la caméra libre à la même position et orientation
    freePosition = position;
    freeSpeed = speed;  // Utiliser la vitesse fournie
    
    // Calculer les angles à partir de la direction (target - position)
    glm::vec3 direction = glm::normalize(target - position);
    
    // Angle horizontal (yaw) - rotation autour de l'axe Y
    freeHorizontalAngle = atan2(direction.x, direction.z);
    
    // Angle vertical (pitch) - rotation verticale
    freeVerticalAngle = asin(direction.y);

    // Initialiser la caméra orbitale à partir de la position fixe
    orbitCenter = target;
    orbitHeightOffset = position.y - target.y;
    orbitRadius = glm::length(glm::vec2(position.x - target.x, position.z - target.z));
    if (orbitRadius < 0.001f) orbitRadius = 1.0f;
    orbitAngle = atan2(position.z - target.z, position.x - target.x);
}

void Camera::setMode(CameraMode mode, GLFWwindow* window) {
    CameraMode previousMode = currentMode;
    currentMode = mode;
    
    // Configurer le curseur selon le mode
    if (mode == FREE_CAMERA) {
        // Activer le mode souris illimité pour la caméra libre
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPos(window, 1024/2, 768/2);
    } else {
        // Désactiver le mode souris pour les autres modes
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (mode == ORBIT_CAMERA) {
        glm::vec3 sourcePos = (previousMode == FREE_CAMERA) ? freePosition : fixedPosition;
        orbitCenter = fixedTarget;
        orbitHeightOffset = sourcePos.y - fixedTarget.y;
        orbitRadius = glm::length(glm::vec2(sourcePos.x - fixedTarget.x, sourcePos.z - fixedTarget.z));
        if (orbitRadius < 0.001f) orbitRadius = 1.0f;
        orbitAngle = atan2(sourcePos.z - fixedTarget.z, sourcePos.x - fixedTarget.x);
    }
}

void Camera::setOrbitSpeed(float speed) {
    if (speed < 0.0f) speed = 0.0f;
    if (speed > 5.0f) speed = 5.0f;
    orbitSpeed = speed;
}

void Camera::update(GLFWwindow* window, float deltaTime) {
    switch(currentMode) {
        case FIXED_CAMERA:
            updateFixedCamera();
            break;
        case FREE_CAMERA:
            updateFreeCamera(window, deltaTime);
            break;
        case ORBIT_CAMERA:
            updateOrbitCamera(window, deltaTime);
            break;
    }
}

void Camera::updateFixedCamera() {
    // Projection matrix
    float aspect = computeAspect(nullptr);
    projectionMatrix = glm::perspective(glm::radians(fieldOfView), aspect, 0.1f, 5000.0f);
    
    // View matrix - utilise les paramètres fixes
    viewMatrix = glm::lookAt(fixedPosition, fixedTarget, fixedUp);
}

void Camera::updateFreeCamera(GLFWwindow* window, float deltaTime) {
    // Gestion de la souris
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    
    // Reset mouse position for next frame
    glfwSetCursorPos(window, 1024/2, 768/2);
    
    // Compute new orientation
    freeHorizontalAngle += freeMouseSpeed * float(1024/2 - xpos);
    freeVerticalAngle   += freeMouseSpeed * float(768/2 - ypos);
    
    // Limiter l'angle vertical pour éviter le retournement
    if (freeVerticalAngle > 1.5f) freeVerticalAngle = 1.5f;
    if (freeVerticalAngle < -1.5f) freeVerticalAngle = -1.5f;
    
    // Direction : Spherical coordinates to Cartesian coordinates conversion
    glm::vec3 direction(
        cos(freeVerticalAngle) * sin(freeHorizontalAngle), 
        sin(freeVerticalAngle),
        cos(freeVerticalAngle) * cos(freeHorizontalAngle)
    );
    
    // Right vector
    glm::vec3 right = glm::vec3(
        sin(freeHorizontalAngle - 3.14f/2.0f), 
        0,
        cos(freeHorizontalAngle - 3.14f/2.0f)
    );
    
    // Up vector
    glm::vec3 up = glm::cross(right, direction);
    
    // Contrôles clavier WASD
    // W : Avancer
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        freePosition += direction * deltaTime * freeSpeed;
    }
    // S : Reculer
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        freePosition -= direction * deltaTime * freeSpeed;
    }
    // D : Déplacement à droite
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        freePosition += right * deltaTime * freeSpeed;
    }
    // A : Déplacement à gauche
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        freePosition -= right * deltaTime * freeSpeed;
    }
    // Espace : Monter
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
        freePosition += up * deltaTime * freeSpeed;
    }
    // Shift gauche : Descendre
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
        freePosition -= up * deltaTime * freeSpeed;
    }
    
    // Projection matrix
    float aspect = computeAspect(window);
    projectionMatrix = glm::perspective(glm::radians(fieldOfView), aspect, 0.1f, 5000.0f);
    
    // Camera matrix
    viewMatrix = glm::lookAt(
        freePosition,              // Position de la caméra
        freePosition + direction,  // Point regardé
        up                         // Vecteur up
    );
}

void Camera::updateOrbitCamera(GLFWwindow* window, float deltaTime) {
    (void)window;

    orbitAngle += orbitSpeed * deltaTime;

    glm::vec3 orbitPosition(
        orbitCenter.x + cos(orbitAngle) * orbitRadius,
        orbitCenter.y + orbitHeightOffset,
        orbitCenter.z + sin(orbitAngle) * orbitRadius
    );

    // Projetion matrix
    float aspect = computeAspect(window);
    projectionMatrix = glm::perspective(glm::radians(fieldOfView), aspect, 0.1f, 5000.0f);

    // View matrix
    viewMatrix = glm::lookAt(orbitPosition, orbitCenter, fixedUp);
}
