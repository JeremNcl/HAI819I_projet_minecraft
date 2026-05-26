#pragma once
#ifndef MONSTERINTERACTIONSYSTEM_HPP
#define MONSTERINTERACTIONSYSTEM_HPP

#include "../registry.hpp"
#include "ecs/components/monster.hpp"
#include "ecs/components/ia.hpp"
#include "ecs/components/player.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/velocity.hpp"
#include "ecs/components/rigidBody.hpp"
#include "ecs/components/collider.hpp"
#include "ecs/components/inputReceiver.hpp"
#include "ecs/components/mesh.hpp"
#include <vector>
#include <GL/glew.h>
#include "engine/render/mesh.hpp"

class MonsterInteractionSystem {
    public:

        void update(Registry& _registry, float _deltaTime);
};

#endif