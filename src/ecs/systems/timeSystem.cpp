#include "timeSystem.hpp"

void TimeSystem::update(Registry& registry, float deltaTime) {
    auto view = registry.view<TimeComponent>();
    
    if (view.isEmpty()) {
        return;
    }
    
    for (EntityID entity : view) {
        TimeComponent& time = registry.getComponent<TimeComponent>(entity);
        
        // Update dayTime unless paused
        if (!time.paused) {
            time.dayTime += time.daySpeed * deltaTime;
            
            // Wrap around to [0, 1]
            if (time.dayTime >= 1.0f) {
                time.dayTime -= 1.0f;
            } else if (time.dayTime < 0.0f) {
                time.dayTime += 1.0f;
            }
        }
    }
}
