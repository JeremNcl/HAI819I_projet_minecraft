#include "lightingCalculationSystem.hpp"
#include <glm/gtc/constants.hpp>
#include <cmath>

// ============================================================================
// Helper Functions
// ============================================================================

float LightingCalculationSystem::inverseLerp(float a, float b, float t) {
    if (b == a) return 0.0f;
    return glm::clamp((t - a) / (b - a), 0.0f, 1.0f);
}

float LightingCalculationSystem::getSunAngle(float t) {
    return (t - 0.25f) * 2.0f * glm::pi<float>();
}

// ============================================================================
// Lighting Calculation Functions (Migrated from main.cpp)
// ============================================================================

glm::vec3 LightingCalculationSystem::computeLightColorFromTime(float t, bool useReducedAmbient) {
    glm::vec3 dayLight    = useReducedAmbient ? glm::vec3(3.0f, 2.9f, 2.8f) : glm::vec3(2.5f, 2.4f, 2.3f);
    glm::vec3 sunsetLight = glm::vec3(2.5f, 1.2f, 0.4f);
    glm::vec3 duskLight   = glm::vec3(0.8f, 0.2f, 0.2f);
    glm::vec3 nightLight  = glm::vec3(0.15f, 0.20f, 0.35f);

    glm::vec3 color;
    if (t < 0.150f) color = nightLight;
    else if (t < 0.200f) color = glm::mix(nightLight, duskLight, inverseLerp(0.150f, 0.200f, t));
    else if (t < 0.250f) color = glm::mix(duskLight, dayLight, inverseLerp(0.200f, 0.250f, t));
    else if (t < 0.750f) color = dayLight;
    else if (t < 0.800f) color = glm::mix(dayLight, sunsetLight, inverseLerp(0.750f, 0.800f, t));
    else if (t < 0.850f) color = glm::mix(sunsetLight, nightLight, inverseLerp(0.800f, 0.850f, t));
    else color = nightLight;

    float angle = getSunAngle(t);
    float elevation = glm::sin(angle);
    float horizonFade = glm::smoothstep(0.0f, 0.15f, glm::abs(elevation));

    return color * horizonFade;
}

glm::vec3 LightingCalculationSystem::computeLightDirectionFromTime(float t) {
    float angle = getSunAngle(t);
    float elevation = glm::sin(angle);
    float azimuth = glm::cos(angle);
    glm::vec3 direction = glm::normalize(glm::vec3(azimuth, elevation, 0.0f));

    if (elevation < 0.0f) {
        direction = -direction;
    }

    direction.y = glm::max(direction.y, 0.05f);
    return glm::normalize(direction);
}

float LightingCalculationSystem::computeAmbientStrengthFromTime(float t, bool useReducedAmbient) {
    static constexpr float kAmbientSoft = 0.055f;
    static constexpr float kAmbientCrisp = 0.035f;
    
    float nightDarkness = 0.10f;
    float dayValue = useReducedAmbient ? kAmbientCrisp : kAmbientSoft;

    if (t < 0.175f) return nightDarkness;
    if (t < 0.250f) return glm::mix(nightDarkness, dayValue, inverseLerp(0.175f, 0.250f, t));
    if (t < 0.750f) return dayValue;
    if (t < 0.825f) return glm::mix(dayValue, nightDarkness, inverseLerp(0.750f, 0.825f, t));
    return nightDarkness;
}

glm::vec3 LightingCalculationSystem::computeAmbientSkyColorFromTime(float t, bool useReducedAmbient) {
    glm::vec3 daySky     = useReducedAmbient ? glm::vec3(0.15f, 0.35f, 0.75f) : glm::vec3(0.25f, 0.45f, 0.85f);
    glm::vec3 goldenHour = glm::vec3(0.85f, 0.60f, 0.30f);
    glm::vec3 sunset     = glm::vec3(0.90f, 0.35f, 0.15f);
    glm::vec3 redDusk    = glm::vec3(0.55f, 0.10f, 0.15f);
    glm::vec3 nightSky   = glm::vec3(0.005f, 0.015f, 0.04f);

    if (t < 0.175f) return nightSky;
    if (t < 0.200f) return glm::mix(nightSky, redDusk, inverseLerp(0.175f, 0.200f, t));
    if (t < 0.225f) return glm::mix(redDusk, goldenHour, inverseLerp(0.200f, 0.225f, t));
    if (t < 0.250f) return glm::mix(goldenHour, daySky, inverseLerp(0.225f, 0.250f, t));
    if (t < 0.750f) return daySky;
    if (t < 0.775f) return glm::mix(daySky, goldenHour, inverseLerp(0.750f, 0.775f, t));
    if (t < 0.800f) return glm::mix(goldenHour, sunset, inverseLerp(0.775f, 0.800f, t));
    if (t < 0.825f) return glm::mix(sunset, redDusk, inverseLerp(0.800f, 0.825f, t));
    if (t <= 1.0f) return glm::mix(redDusk, nightSky, inverseLerp(0.825f, 0.850f, t));

    return nightSky;
}

glm::vec3 LightingCalculationSystem::computeHorizonColorFromTime(float t, bool useReducedAmbient) {
    glm::vec3 dayHorizon     = useReducedAmbient ? glm::vec3(0.40f, 0.50f, 0.65f) : glm::vec3(0.65f, 0.75f, 0.85f);
    glm::vec3 goldenHorizon  = glm::vec3(0.95f, 0.75f, 0.45f);
    glm::vec3 sunsetHorizon  = glm::vec3(0.95f, 0.40f, 0.10f);
    glm::vec3 duskHorizon    = glm::vec3(0.40f, 0.15f, 0.20f);
    glm::vec3 nightHorizon   = glm::vec3(0.02f, 0.04f, 0.08f);

    if (t < 0.175f) return nightHorizon;
    if (t < 0.200f) return glm::mix(nightHorizon, duskHorizon, inverseLerp(0.175f, 0.200f, t));
    if (t < 0.225f) return glm::mix(duskHorizon, goldenHorizon, inverseLerp(0.200f, 0.225f, t));
    if (t < 0.250f) return glm::mix(goldenHorizon, dayHorizon, inverseLerp(0.225f, 0.250f, t));
    if (t < 0.750f) return dayHorizon;
    if (t < 0.775f) return glm::mix(dayHorizon, goldenHorizon, inverseLerp(0.750f, 0.775f, t));
    if (t < 0.800f) return glm::mix(goldenHorizon, sunsetHorizon, inverseLerp(0.775f, 0.800f, t));
    if (t < 0.825f) return glm::mix(sunsetHorizon, duskHorizon, inverseLerp(0.800f, 0.825f, t));
    if (t <= 1.0f) return glm::mix(duskHorizon, nightHorizon, inverseLerp(0.825f, 0.850f, t));

    return nightHorizon;
}

glm::vec3 LightingCalculationSystem::computeAmbientGroundColorFromTime(float t, bool useReducedAmbient) {
    glm::vec3 dayGround    = useReducedAmbient ? glm::vec3(0.08f, 0.07f, 0.05f) : glm::vec3(0.12f, 0.10f, 0.08f);
    glm::vec3 sunsetGround = glm::vec3(0.18f, 0.10f, 0.06f);
    glm::vec3 nightGround  = glm::vec3(0.01f, 0.015f, 0.02f);

    if (t < 0.175f) return nightGround;
    if (t < 0.250f) return glm::mix(nightGround, dayGround, inverseLerp(0.175f, 0.250f, t));
    if (t < 0.750f) return dayGround;
    if (t < 0.825f) return glm::mix(dayGround, sunsetGround, inverseLerp(0.750f, 0.825f, t));
    if (t <= 1.0f)  return glm::mix(sunsetGround, nightGround, inverseLerp(0.825f, 0.850f, t));

    return nightGround;
}

float LightingCalculationSystem::computeExposureFromTime(float t) {
    float angle = (t - 0.25f) * 2.0f * glm::pi<float>();
    float elevation = glm::sin(angle);

    float noonExposure = 0.8f;
    float twilightExposure = 1.2f;
    float nightExposure = 1.8f;

    if (elevation > 0.1f) {
        return glm::mix(twilightExposure, noonExposure, inverseLerp(0.1f, 1.0f, elevation));
    } else if (elevation > -0.1f) {
        return glm::mix(nightExposure, twilightExposure, inverseLerp(-0.1f, 0.1f, elevation));
    } else {
        return glm::mix(nightExposure, 2.2f, inverseLerp(-0.1f, -1.0f, elevation));
    }
}

// ============================================================================
// Main System Update
// ============================================================================

void LightingCalculationSystem::update(Registry& registry) {
    // Find TimeComponent
    auto timeView = registry.view<TimeComponent>();
    if (timeView.isEmpty()) {
        return;
    }
    EntityID timeEntity = *timeView.begin();
    TimeComponent& time = registry.getComponent<TimeComponent>(timeEntity);

    // Find or create LightingStateComponent (should be on same or different entity)
    auto lightingView = registry.view<LightingStateComponent>();
    if (lightingView.isEmpty()) {
        return;
    }
    EntityID lightingEntity = *lightingView.begin();
    LightingStateComponent& lighting = registry.getComponent<LightingStateComponent>(lightingEntity);

    // Update all lighting state based on current dayTime
    lighting.lightColor = computeLightColorFromTime(time.dayTime, lighting.useReducedAmbient);
    lighting.lightDirection = computeLightDirectionFromTime(time.dayTime);
    lighting.ambientStrength = computeAmbientStrengthFromTime(time.dayTime, lighting.useReducedAmbient);
    lighting.ambientSkyColor = computeAmbientSkyColorFromTime(time.dayTime, lighting.useReducedAmbient);
    lighting.horizonColor = computeHorizonColorFromTime(time.dayTime, lighting.useReducedAmbient);
    lighting.ambientGroundColor = computeAmbientGroundColorFromTime(time.dayTime, lighting.useReducedAmbient);
    lighting.exposure = computeExposureFromTime(time.dayTime);
}
