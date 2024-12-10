#include <optional>
#include <iostream>
#include <algorithm>
#include "realtimescene.h"
#include "objects/realtimeobject.h"
#include "objects/staticobject.h"
#include "glm/ext/matrix_transform.hpp"
#include "utils/helpers.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantParameter"
#define MAX_LIGHTS 16

const unsigned int SHADER_LIGHT_POINT       = 0x1u;
const unsigned int SHADER_LIGHT_DIRECTIONAL = 0x2u;
const unsigned int SHADER_LIGHT_SPOT        = 0x4u;


// GLuint loadCubemap(std::vector<std::string> faces) {
//     GLuint cubemapID;
//     glGenTextures(1, &cubemapID);
//     glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);
//
//     for (GLuint i = 0; i < faces.size(); i++) {
//         // Load each face image using your custom loadImageFromFile function
//         std::unique_ptr<Image> img = loadImageFromFile(faces[i]);
//         if (!img) {
//             std::cerr << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
//             continue;
//         }
//
//         // Upload the image to the corresponding face of the cube map
//         glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
//                      0, GL_RGBA, img->width, img->height,
//                      0, GL_RGBA, GL_UNSIGNED_BYTE, img->data.data());
//     }
//
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//
//     return cubemapID;
// }


std::shared_ptr<RealtimeScene> RealtimeScene::init(int width, int height, const std::string& sceneFilePath,
                                                 float nearPlane, float farPlane,
                                                 std::map<PrimitiveType, std::shared_ptr<PrimitiveMesh>> meshes) {
    RenderData renderData;
    if (!SceneParser::parse(sceneFilePath, renderData)) {
        std::cerr << "Failed to initialize scene: failed to parse scene file" << std::endl;
        return {nullptr};
    }
    if (renderData.lights.size() > MAX_LIGHTS) {
        std::cerr << "Failed to initialize scene: too many lights" << std::endl;
        return {nullptr};
    }
    auto newScene = std::shared_ptr<RealtimeScene>(new RealtimeScene(width, height, nearPlane, farPlane, renderData.globalData, renderData.cameraData, std::move(meshes)));

    // all this below initialization must be done in this factory function, not the constructor, due to needing to pass a shared_ptr
    // to this scene to the objects

    // TODO currently all objects imported from the scene file will be static collidable objects.
    //  ideally we'd extend the scene format to allow it to specify what physics properties an object has
    newScene->m_objects.reserve(renderData.shapes.size() + 1);
    newScene->m_collisionObjects.reserve(renderData.shapes.size() + 1);
    for (const auto& shape : renderData.shapes) {
        auto staticObject = std::make_shared<StaticObject>(shape, newScene);
        auto collisionObject = std::weak_ptr<CollisionObject>(std::static_pointer_cast<CollisionObject>(staticObject));
        auto object = std::static_pointer_cast<RealtimeObject>(staticObject);
        newScene->m_objects.push_back(object);
        newScene->m_collisionObjects.push_back(collisionObject);
    }
    // create player object TODO better way of doing this
    ScenePrimitive playerPrimitive{PrimitiveType::PRIMITIVE_CUBE, SceneMaterial()};
    // start player scaled up by 1 (i.e. 1 unit wide); start player centered at camera
    glm::mat4 playerCTM = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(1.f)), newScene->m_camera->pos());
    RenderShapeData playerShapeData = RenderShapeData{playerPrimitive, playerCTM};
    newScene->m_playerObject = std::make_shared<PlayerObject>(playerShapeData, newScene, newScene->m_camera);
    auto playerCollisionObject = std::weak_ptr<CollisionObject>(std::static_pointer_cast<CollisionObject>(newScene->m_playerObject));
    auto playerRealtimeObject = std::static_pointer_cast<RealtimeObject>(newScene->m_playerObject);
    newScene->m_objects.push_back(playerRealtimeObject);
    newScene->m_collisionObjects.push_back(playerCollisionObject);

    newScene->m_lights.reserve(MAX_LIGHTS);
    for (const auto& light : renderData.lights) {
        // normalizing is important (and faster to do here than on the gpu for every fragment)
        newScene->m_lights.push_back({light.id, light.type, light.color, light.function, light.pos, glm::normalize(light.dir),
                              light.penumbra, light.angle, light.width, light.height});
    }
    //Create skybox object
    ScenePrimitive skyboxPrimitive{PrimitiveType::PRIMITIVE_SKYBOX,
        SceneMaterial{SceneColor{0.1f, 0.1f, 0.1f, 1.f}, SceneColor{1.f, 1.f, 1.f, 1.f}}};
    skyboxPrimitive.material.textureMap.isUsed = true;
    skyboxPrimitive.material.textureMap.filename = "scenefiles/moretextures/stars.png";

    skyboxPrimitive.material.blend = 0.5f;  // Adjust blend factor as needed
    skyboxPrimitive.material.textureMap.repeatU = 1.0f;  // Set U repeat value
    skyboxPrimitive.material.textureMap.repeatV = 1.0f;  // Set V repeat value

    glm::mat4 skyboxCTM = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));  // Scale skybox to surround the scene
    RenderShapeData skyboxShapeData = RenderShapeData{skyboxPrimitive, skyboxCTM};
    auto skyboxObject = std::make_shared<RealtimeObject>(skyboxShapeData, newScene);
    newScene->m_objects.push_back(skyboxObject);
    //Add texture for skybox


    return newScene;
}

RealtimeScene::RealtimeScene(int width, int height, float nearPlane, float farPlane, SceneGlobalData globalData, SceneCameraData cameraData,
                             std::map<PrimitiveType, std::shared_ptr<PrimitiveMesh>> meshes) :
m_width(width), m_height(height), m_globalData(globalData),
m_camera(std::make_shared<Camera>(width, height, cameraData, nearPlane, farPlane)),
m_nearPlane(nearPlane), m_farPlane(farPlane),
m_meshes(std::move(meshes)) {}

void RealtimeScene::tick(double elapsedSeconds) {
    size_t currentSize = m_objects.size();
    for (int i = 0; i < currentSize; i++) {
        m_objects[i]->tick(elapsedSeconds);
        // size of the vector may change during the tick, so we need to check if the object is still valid
        currentSize = m_objects.size();
    }

    // https://stackoverflow.com/a/7958447
    m_objects.erase(
            std::remove_if(m_objects.begin(), m_objects.end(),
                           [](const std::shared_ptr<RealtimeObject>& o) { return o->isQueuedFree(); }),
            m_objects.end());
    // simply remove all empty elements from collisionObjects
    m_collisionObjects.erase(
            std::remove_if(m_collisionObjects.begin(), m_collisionObjects.end(),
                           [](const std::weak_ptr<CollisionObject>& o) { return o.expired(); }),
            m_collisionObjects.end());

}

void RealtimeScene::paintObjects() {
    if (!shaderInitialized()) {
        std::cerr << "Failed to paint objects: shader not initialized" << std::endl;
        return;
    }
    glUseProgram(*m_phongShader);
    passUniformMat4("view", m_camera->viewMatrix());
    passUniformMat4("proj", m_camera->projectionMatrix());
    passUniformInt("numLights", (int) m_lights.size());
    passUniformLightArray("lights", m_lights);
    passUniformVec3("cameraPosWS", m_camera->pos().xyz());
    passUniformFloat("ka", m_globalData.ka);
    passUniformFloat("kd", m_globalData.kd);
    passUniformFloat("ks", m_globalData.ks);
    // set texture slot
    glActiveTexture(GL_TEXTURE0);

    //First paint skybox
    //
    if (m_skyboxObject) {
        // Render the skybox object
        passUniformMat4("model", m_skyboxObject->CTM());  // Pass the skybox model matrix
        passUniformMat3("inverseTransposeModel", m_skyboxObject->inverseTransposeCTM());

        glBindVertexArray(m_skyboxObject->mesh()->vao());
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_skyboxObject->mesh()->vertexData().size() / 3));
        glBindVertexArray(0);
    }
    for (const auto& object : m_objects) {
        if (!object->shouldRender()) {
            continue;
        }
        const SceneMaterial& material = object->material();
        if (object->usesTexture()) {
            if (!object->glTexAllocated()) {
                object->allocateGLTex();
            }
            glBindTexture(GL_TEXTURE_2D, object->glTexID());
            passUniformInt("usesTexture", 1);
            // TODO is it okay to sometimes not pass these uniforms?
            passUniformFloat("blend", material.blend);
            passUniformFloat("repeatU", material.textureMap.repeatU);
            passUniformFloat("repeatV", material.textureMap.repeatV);
        } else {
            passUniformInt("usesTexture", 0);
        }
        passUniformMat4("model", object->CTM());
        passUniformMat3("inverseTransposeModel", object->inverseTransposeCTM());
        passUniformVec3("cAmbient", material.cAmbient.xyz());
        passUniformVec3("cDiffuse", material.cDiffuse.xyz());
        passUniformVec3("cSpecular", material.cSpecular.xyz());
        passUniformFloat("shininess", material.shininess);
        glBindVertexArray(object->mesh()->vao());
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei) (object->mesh()->vertexData().size() / 3));
        glBindVertexArray(0);
        if (object->usesTexture()) {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    glUseProgram(0);
}




void RealtimeScene::passUniformMat4(const char* name, const glm::mat4& mat) {
    helpers::passUniformMat4(m_phongShader.value(), name, mat);
}

void RealtimeScene::passUniformMat3(const char* name, const glm::mat3& mat) {
    helpers::passUniformMat3(m_phongShader.value(), name, mat);
}

void RealtimeScene::passUniformFloat(const char* name, float value) {
    helpers::passUniformFloat(m_phongShader.value(), name, value);
}

void RealtimeScene::passUniformInt(const char* name, int value) {
    helpers::passUniformInt(m_phongShader.value(), name, value);
}

void RealtimeScene::passUniformVec3(const char* name, const glm::vec3& vec) {
    helpers::passUniformVec3(m_phongShader.value(), name, vec);
}

void RealtimeScene::passUniformVec3Array(const char* name, const std::vector<glm::vec3>& vecs) {
    helpers::passUniformVec3Array(m_phongShader.value(), name, vecs);
}

void RealtimeScene::passUniformLightArray(const char* name, const std::vector<SceneLightData>& lights) {
    // hmm this will cause allocations every time TODO maybe optimize
    std::string lightName = std::string(name);
    for (int i = 0; i < lights.size(); i++) {
        passUniformLight((lightName + "[" + std::to_string(i) + "]").c_str(), lights[i]);
    }
}

void RealtimeScene::passUniformLight(const char* name, SceneLightData type) {
    // hmm this will cause allocations every time TODO maybe optimize
    std::string lightName = std::string(name);
    glUniform1ui(getUniformLocation((lightName + ".type").c_str()), lightTypeToUniform(type.type));
    glUniform3fv(getUniformLocation((lightName + ".color").c_str()), 1, &type.color.xyz()[0]);
    glUniform3fv(getUniformLocation((lightName + ".pos").c_str()), 1, &type.pos.xyz()[0]);
    glUniform3fv(getUniformLocation((lightName + ".dir").c_str()), 1, &type.dir.xyz()[0]);
    glUniform3fv(getUniformLocation((lightName + ".function").c_str()), 1, &type.function[0]);
    glUniform1f(getUniformLocation((lightName + ".penumbra").c_str()), type.penumbra);
    glUniform1f(getUniformLocation((lightName + ".angle").c_str()), type.angle);
}

GLint RealtimeScene::getUniformLocation(const char* name, bool checkValidLoc) const {
    return helpers::getUniformLocation(m_phongShader.value(), name, checkValidLoc);
}

void RealtimeScene::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;
    m_camera->setSceneDimensions(width, height);
}


void RealtimeScene::updateSettings(float nearPlane, float farPlane) {
    if (farPlane != m_farPlane || nearPlane != m_nearPlane) {
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
        m_camera->setNearAndFar(nearPlane, farPlane);
    }
}

void RealtimeScene::initShader(GLuint shader) {
    m_phongShader = shader;
}

bool RealtimeScene::shaderInitialized() const {
    return m_phongShader.has_value();
}


int RealtimeScene::width() const {
    return m_width;
}

int RealtimeScene::height() const {
    return m_height;
}

GLuint RealtimeScene::lightTypeToUniform(LightType type) {
    switch (type) {
        case LightType::LIGHT_POINT:
            return SHADER_LIGHT_POINT;
        case LightType::LIGHT_DIRECTIONAL:
            return SHADER_LIGHT_DIRECTIONAL;
        case LightType::LIGHT_SPOT:
            return SHADER_LIGHT_SPOT;
        default:
            throw std::runtime_error("Invalid light type");
    }
}

void RealtimeScene::keyPressEvent(int key) {
    m_playerObject->keyPressEvent(key);
}

void RealtimeScene::keyReleaseEvent(int key) {
    m_playerObject->keyReleaseEvent(key);
}

void RealtimeScene::mousePressEvent(int button) {
    m_playerObject->mousePressEvent(button);

}

void RealtimeScene::mouseReleaseEvent(int button) {
    m_playerObject->mouseReleaseEvent(button);
}

void RealtimeScene::mouseMoveEvent(double xpos, double ypos) {
    m_playerObject->mouseMoveEvent(xpos, ypos);
}

std::shared_ptr<RealtimeObject> RealtimeScene::addObject(PrimitiveType type, const glm::mat4& ctm, const SceneMaterial& material,
                              RealtimeObjectType objType) {
    switch (objType) {
        case RealtimeObjectType::OBJECT:
            return addObject(std::make_unique<RealtimeObject>(RenderShapeData{ScenePrimitive{type, material}, ctm}, shared_from_this()));
        case RealtimeObjectType::STATIC:
            return addObject(std::make_unique<StaticObject>(RenderShapeData{ScenePrimitive{type, material}, ctm}, shared_from_this()));
    }
    throw std::runtime_error("Invalid object type");
}

std::shared_ptr<RealtimeObject> RealtimeScene::addObject(std::unique_ptr<RealtimeObject> object) {
    std::shared_ptr<RealtimeObject> objectShared = std::move(object);
    if (!objectShared) {
        std::cerr << "Failed to add object to scene: object is null" << std::endl;
        return {nullptr};
    }
    // tests if object is a subclass of CollisionObject
    // if so, we have to add it to the collision objects list
    std::shared_ptr<CollisionObject> maybeCollisionObject = std::dynamic_pointer_cast<CollisionObject>(objectShared);
    if (maybeCollisionObject) {
        std::weak_ptr<CollisionObject> collisionObject = std::weak_ptr<CollisionObject>(maybeCollisionObject);
        m_collisionObjects.push_back(collisionObject);
    }
    m_objects.push_back(objectShared);
    return objectShared;
}


const std::map<PrimitiveType, std::shared_ptr<PrimitiveMesh>>& RealtimeScene::meshes() const {
    return m_meshes;
}

const std::vector<std::weak_ptr<CollisionObject>>& RealtimeScene::collisionObjects() const {
    return m_collisionObjects;
}

void RealtimeScene::finish() {
    for (const auto& object : m_objects) {
        object->finish();
    }
}

#pragma clang diagnostic pop
