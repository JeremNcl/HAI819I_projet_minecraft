#pragma once
#ifndef MONSTER_HPP
#define MONSTER_HPP

#include "component.hpp"

struct MonsterComponent : public Component {
    float detectionRange = 20.f;
    float loseTargetRange = 30.f;
    float attackRange = 1.5f;

    float wanderingRange = 10.f;
    float wanderingTimer = 6.f;
    float currentWanderingTimer = 0.f;

    bool isChasing = false;
};

#endif