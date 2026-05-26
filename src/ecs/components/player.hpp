#pragma once
#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "chunk.hpp"
#include "component.hpp"

struct PlayerComponent : public Component {

    float reach = 6.f;
    bool canBreakBlocks = true;
    bool canPlaceBlocks = true;

    VoxelType currentBloc = VoxelType::STONE;

};

#endif