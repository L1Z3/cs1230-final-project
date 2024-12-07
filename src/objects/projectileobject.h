 #pragma once

 #include "collisionobject.h"
 #include <memory>

 class ProjectileObject : public CollisionObject {
 public:
     ProjectileObject(const RenderShapeData& data,
                      const std::shared_ptr<RealtimeScene>& scene,
                      const glm::vec3& direction,
                      float speed,
                      float maxDistance,
                      bool isBullet);

     void tick(double elapsedSeconds) override;
     void collisionSphereEffect();
 private:
     glm::vec3 m_direction;      // Unit direction vector for projectile movement
     float m_speed;              // Speed of the projectile
     float m_traveledDistance;   // Distance traveled by the projectile
     float m_maxDistance;        // Maximum distance the projectile can travel before being destroyed
     float m_isBullet;

     typedef CollisionObject super;
 };
