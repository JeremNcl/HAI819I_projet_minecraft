#pragma once
#ifndef DELTATIMESYSTEM_HPP
#define DELTATIMESYSTEM_HPP

#include "../registry.hpp"
#include "../components/deltaTime.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>

class DeltaTimeSystem {
    public:
        void update(DeltaTimeComponent& _deltaTime);
};

#endif