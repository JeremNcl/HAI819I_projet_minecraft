#include "deltaTimeSystem.hpp"

void DeltaTimeSystem::update(DeltaTimeComponent& _deltaTime) {

    float currentFrame = static_cast<float>(glfwGetTime());

    float currentDeltaTime = currentFrame - _deltaTime.lastFrame;

    _deltaTime.lastFrame = currentFrame;

    _deltaTime.rawDeltaTime = currentDeltaTime;

    _deltaTime.deltaTime = std::min(currentDeltaTime, _deltaTime.maxDeltaTime) * _deltaTime.timeScale;
}