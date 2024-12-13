#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-auto"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include "playerobject.h"
#include "realtimescene.h"
#include "projectileobject.h"
#include "utils/helpers.h"


PlayerObject::PlayerObject(const RenderShapeData& data,
                           const std::shared_ptr<RealtimeScene>& scene,
                           std::shared_ptr<Camera> camera,
                           std::shared_ptr<std::vector<SceneLightData>> lights)
    : super(data, scene), m_camera(std::move(camera)), m_prev_mouse_pos(std::nullopt), m_lights(std::move(lights)), m_savedLight(std::nullopt) {
    // player shouldn't render by default
    setShouldRender(false);
    // don't collide with projectiles
    setCollisionFilter([](std::shared_ptr<CollisionObject> object) {
        // Don't collide with the player
        if (std::dynamic_pointer_cast<ProjectileObject>(object)) {
            return false;
        } else {
            return true;
        }
    });
}

void PlayerObject::translate(const glm::vec3& translation) {
    super::translate(translation);
    m_camera->translate(translation);
}

void PlayerObject::tick(double elapsedSeconds) {
    super::tick(elapsedSeconds);

    float deltaTime = (float) elapsedSeconds;
    glm::vec3 accelVec = glm::vec3(0.f);
    // Use deltaTime and m_keyMap here to move around
    glm::vec3 look2D = glm::normalize(glm::vec3(m_camera->look().x, 0.f, m_camera->look().z));
    if (m_keyMap[GLFW_KEY_W]) {
        accelVec += PLAYER_MOVE_ACCEL_WITH_FRICTION * deltaTime * look2D;
    }
    if (m_keyMap[GLFW_KEY_S]) {
        accelVec -= PLAYER_MOVE_ACCEL_WITH_FRICTION * deltaTime * look2D;
    }
    if (m_keyMap[GLFW_KEY_A]) {
        accelVec += PLAYER_MOVE_ACCEL_WITH_FRICTION * deltaTime * glm::normalize(glm::cross(m_camera->up(), look2D));
    }
    if (m_keyMap[GLFW_KEY_D]) {
        accelVec -= PLAYER_MOVE_ACCEL_WITH_FRICTION * deltaTime * glm::normalize(glm::cross(m_camera->up(), look2D));
    }
    if (m_onGround && m_keyMap[GLFW_KEY_SPACE]) {
        accelVec.y += PLAYER_JUMP_SPEED * deltaTime;
    }
    m_velocity += accelVec;
    glm::vec3 horizAccelVec = glm::vec3(accelVec.x, 0, accelVec.z);
    glm::vec3 horizVel = glm::vec3(m_velocity.x, 0, m_velocity.z);
    float horizSpeed = glm::length(horizVel);
    if (horizSpeed > 0.f) {
        if (horizSpeed < PLAYER_FRICTION_ACCEL * deltaTime) {
            horizVel = glm::vec3(0.f);
        } else {
            horizVel -= PLAYER_FRICTION_ACCEL * glm::normalize(horizVel) * deltaTime;
        }
    }

    if (horizSpeed > PLAYER_MAX_HORIZ_SPEED) {
        horizVel = glm::normalize(horizVel) * PLAYER_MAX_HORIZ_SPEED;
    }
    m_velocity = glm::vec3(horizVel.x, m_velocity.y, horizVel.z);

    if (!m_onGround) {
        m_velocity.y -= m_gravity * deltaTime;
    }

    if (m_velocity.y < -PLAYER_TERMINAL_VELOCITY) {
        m_velocity.y = -PLAYER_TERMINAL_VELOCITY;
    }

    glm::vec3 translation = m_velocity * deltaTime;
    auto collisionInfoOpt = getCollisionInfo(translation);

    // reset velocity in direction of collision
    if (collisionInfoOpt.has_value()) {
        glm::vec3 collisionMovementDir = glm::normalize(collisionInfoOpt->collisionCorrectionVec);
        glm::vec3 projOfVelOnCollisionMovementDir = helpers::projectAontoB(m_velocity, collisionMovementDir);
        m_velocity -= projOfVelOnCollisionMovementDir;
        if (collisionMovementDir.y > 0.f) {
            m_onGround = true;
        }
        translation += collisionInfoOpt->collisionCorrectionVec;
    }
    // example usage of adding object to the scene
    // TODO remove this in the future
    if (m_keyMap[GLFW_KEY_E]) {
        m_keyMap[GLFW_KEY_E] = false;

        scene()->addObject(PrimitiveType::PRIMITIVE_CONE, glm::translate(glm::mat4(1.f), m_camera->pos() + 2.f * m_camera->look()),
                           SceneMaterial{SceneColor{0.1f, 0.1f, 0.1f, 1.f}, SceneColor{1.f, 1.f, 1.f, 1.f}},
                           RealtimeObjectType::STATIC);

    }
    // example usage of removing object from scene
    // TODO remove this in the future
    if (m_keyMap[GLFW_KEY_R]) {
        auto collisionInfoLookOpt = getCollisionInfo(m_camera->look());
        if (collisionInfoLookOpt.has_value()) {
            m_keyMap[GLFW_KEY_R] = false;
            (*collisionInfoLookOpt->objects.begin())->queueFree();
        }
    }

    // reset onGround if we are not on the ground (i.e. we can freely move in y dir without collision)
    if (m_onGround && !getCollisionInfo(glm::vec3(0.f, -EPSILON, 0.f)).has_value()) {
        m_onGround = false;
    }



    translate(translation);

    if (m_mouseButtonMap[GLFW_MOUSE_BUTTON_LEFT]) {
        spawnBullet();
        m_mouseButtonMap[GLFW_MOUSE_BUTTON_LEFT] = false;
    }

    // THIS IS JANK: PLAYER SETS FIRST LIGHT IN SCENE TO PLAYER POSITION
    if (!m_lights->empty()) {
        if (!m_savedLight.has_value()) {
            m_savedLight = m_lights->at(0);
        }
        m_lights->at(0).pos = glm::vec4(m_camera->pos(), 1.f);
        m_lights->at(0).dir = glm::vec4(m_camera->look(), 0.f);
    }

    if (m_keyMap[GLFW_KEY_F]) {
        m_keyMap[GLFW_KEY_F] = false;
        m_flashLightOn = !m_flashLightOn;
        if (!m_lights->empty() && m_flashLightOn) {
            m_lights->at(0).color = m_savedLight->color;
        } else {
            m_lights->at(0).color = SceneColor{0.f, 0.f, 0.f, 1.f};
        }
    }

}

void PlayerObject::spawnBullet() {
    // Create a projectile and add it to the scene
    // glm::vec3 spawnPosition = m_camera->pos() + m_camera->look() * 1.5f; // Spawn slightly in front of the player
    glm::vec3 spawnPosition = m_camera->pos();
    glm::vec3 direction = m_camera->look();

    // Create the projectile's render shape data
    ScenePrimitive projectilePrimitive{PrimitiveType::PRIMITIVE_SPHERE,
                                       SceneMaterial{SceneColor{0.1f, 0.1f, 0.1f, 1.f}, SceneColor{1.f, 1.f, 1.f, 1.f}}};

    projectilePrimitive.material.textureMap.isUsed = true;;
    projectilePrimitive.material.textureMap.filename = "scenefiles/moretextures/green_halo.jpg";
    projectilePrimitive.material.textureMap.repeatU = 1.0f;  // Set U repeat value
    projectilePrimitive.material.textureMap.repeatV = 1.0f;  // Set V repeat value

    projectilePrimitive.material.blend = 0.5f;  // Adjust blend factor as needed



    glm::mat4 projectileCTM =  glm::translate(glm::mat4(1.f), spawnPosition);
    projectileCTM = glm::scale(projectileCTM, glm::vec3(0.2f));  // Scaling factor (make it smaller)

    RenderShapeData projectileData{projectilePrimitive, projectileCTM};

    // Add projectile to the scene
    scene()->addObject(std::make_unique<ProjectileObject>(
            projectileData, scene(), direction, 10.f, 50.f, true)); // Speed: 10, Max Distance: 50
}

void PlayerObject::keyPressEvent(int key) {
    m_keyMap[key] = true;
}

void PlayerObject::keyReleaseEvent(int key) {
    m_keyMap[key] = false;
}

void PlayerObject::mousePressEvent(int button) {
    m_mouseButtonMap[button] = true;
}

void PlayerObject::mouseReleaseEvent(int button) {
    m_mouseButtonMap[button] = false;
}

void PlayerObject::mouseMoveEvent(double xpos, double ypos) {
    if (!m_prev_mouse_pos.has_value()) {
        m_prev_mouse_pos = glm::dvec2(xpos, ypos);
        return;
    }
    double deltaX = xpos - m_prev_mouse_pos->x;
    double deltaY = ypos - m_prev_mouse_pos->y;

    // TODO implement normal FPS up/down camera limits
    // Use deltaX and deltaY here to rotate (negate them because idk)
    m_camera->rotate(glm::vec3(0.f, 1.f, 0.f), (float) -deltaX * ROTATE_SENSITIVITY);
    m_camera->rotate(glm::normalize(glm::cross(m_camera->look(), m_camera->up())), (float) -deltaY * ROTATE_SENSITIVITY);

    m_prev_mouse_pos = glm::dvec2(xpos, ypos);
}

#pragma clang diagnostic pop
