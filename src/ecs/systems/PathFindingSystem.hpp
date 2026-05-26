#pragma once
#include "ecs/registry.hpp"
#include "ecs/components/transform.hpp"
#include "modules/pathfinding/PathFinder3D.hpp"
#include "ecs/components/ia.hpp"

class PathFindingSystem {
private:
    PathFinder3D m_pathfinder;

public:
    void update(Registry& registry) {
        auto view = registry.view<IAComponent, TransformComponent>();

        for (EntityID entity : view) {
            auto& ai = registry.getComponent<IAComponent>(entity);
            const auto& transform = registry.getComponent<TransformComponent>(entity);

            if (ai.currentPath.empty()) {
                ai.currentPath = m_pathfinder.FindPath(glm::ivec3(transform.position), ai.target, registry);
            }
        }
    }
};