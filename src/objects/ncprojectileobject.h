 #pragma once

#include "realtimeobject.h"
#include <memory>

class NCProjectileObject : public RealtimeObject {
public:
    NCProjectileObject(const RenderShapeData& data,
                     const std::shared_ptr<RealtimeScene>& scene,
                     const glm::vec3& direction,
                     float speed,
                     float maxDistance);

    void tick(double elapsedSeconds) override;
private:
    glm::vec3 m_direction;      // Unit direction vector for projectile movement
    float m_speed;              // Speed of the projectile
    float m_traveledDistance;   // Distance traveled by the projectile
    float m_maxDistance;        // Maximum distance the projectile can travel before being destroyed

    typedef RealtimeObject super;
};
