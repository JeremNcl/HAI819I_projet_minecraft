#pragma once
#ifndef TIME_COMPONENT_HPP
#define TIME_COMPONENT_HPP

#include "component.hpp"

struct TimeComponent : public Component {
    // Day/Night cycle time normalized [0, 1]
    // 0.0 = midnight
    // 0.25 = sunrise
    // 0.5 = noon
    // 0.75 = sunset
    // 1.0 = next midnight
    float dayTime = 0.0f;
    
    // Speed of day cycle (units per second, fraction of day per second)
    float daySpeed = 0.02f;
    
    // Pause flag
    bool paused = false;
    
    // Ambient lighting preset: SOFT (higher ambient, less contrast) vs CRISP (lower ambient, more contrast)
    enum class AmbientPreset { SOFT, CRISP };
    AmbientPreset ambientPreset = AmbientPreset::SOFT;
    
    TimeComponent() = default;
};

#endif
