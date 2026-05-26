#pragma once
#ifndef IA_HPP
#define IA_HPP

#include <glm/glm.hpp>
#include <vector>
#include "component.hpp"

struct IAComponent : public Component {
    glm::ivec3 target = glm::ivec3(0);
    std::vector<glm::ivec3> currentPath;
};

#endif