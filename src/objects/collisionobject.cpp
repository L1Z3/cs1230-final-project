#include <iostream>
#include <utility>
#include "collisionobject.h"
#include "realtimescene.h"

CollisionObject::CollisionObject(const RenderShapeData& data,
                                 const std::shared_ptr<RealtimeScene>& scene)
        : super(data, scene), m_aabb(mesh()->computeAABB(CTM())) {}


glm::vec3 CollisionObject::translateAndCollide(const glm::vec3& translation) {
    glm::vec3 actualTranslation;
    auto collisionInfoOpt = getCollisionInfo(translation);
    if (collisionInfoOpt.has_value()) {
        actualTranslation = translation + collisionInfoOpt->collisionCorrectionVec;
    } else {
        actualTranslation = translation;
    }
    translate(actualTranslation);
    return actualTranslation;
}

void CollisionObject::translate(const glm::vec3& translation) {
    super::translate(translation);
    m_aabb.translate(translation);
}

std::optional<CollisionInfo> CollisionObject::getCollisionInfo(const glm::vec3& targetTranslation) const {
    AABB movedAABB = m_aabb;
    movedAABB.translate(targetTranslation);
    for (const auto& objectWeak : scene()->collisionObjects()) {
        auto object = objectWeak.lock();
        if (!object) {
            std::cerr << "Collision object weak pointer expired! This shouldn't happen..." << std::endl;
            continue;
        }
        if (object.get() == this) {
            continue;
        }
        const auto& otherAABB = object->aabb();
        if (movedAABB.collides(otherAABB)) {
            return CollisionInfo{movedAABB.getCollisionMoveVec(otherAABB), object};
        }
    }
    return std::nullopt;
}


const AABB& CollisionObject::aabb() const {
    return m_aabb;
}