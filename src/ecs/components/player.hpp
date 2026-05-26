#pragma once
#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "component.hpp"

struct PlayerComponent : public Component {

    float reach = 6.f;
    bool canBreakBlocks = true;
    bool canPlaceBlocks = true;

};

#endif