#include "glm/ext/matrix_transform.hpp"
#include "realtimeobject.h"
#include "realtimescene.h"

RealtimeObject::RealtimeObject(const RenderShapeData& data, const std::shared_ptr<RealtimeScene>& scene) :
m_mesh(scene->meshes().at(data.primitive.type)), m_ctm(data.ctm),
m_inverseOfTranspose3x3CTM(glm::inverse(glm::transpose(glm::mat3(data.ctm)))),
m_material(data.primitive.material), m_type(data.primitive.type), m_shouldRender(true), m_scene(std::weak_ptr(scene)),
m_queuedFree(false) {
    if (m_material.textureMap.isUsed) {
        std::unique_ptr<Image> image = loadImageFromFile(m_material.textureMap.filename);
        if (image) {
            m_texture = std::move(image);
            if (m_material.blend < 0 || m_material.blend > 1) {
                std::cerr << "Invalid blend value for texture map. Must be between 0 and 1." << std::endl;
                m_texture = nullptr;
            }
            return;
        }
    }
    m_texture = nullptr;
}

void RealtimeObject::translate(const glm::vec3& translation) {
    // LOL GLM TRANSLATE RIGHT MULTIPLIES
    glm::mat4 transMatrix = glm::translate(glm::mat4(1.f), translation);
    m_ctm = transMatrix * m_ctm;
    m_inverseOfTranspose3x3CTM = glm::inverse(glm::transpose(glm::mat3(m_ctm)));
}

// default physics tick does nothing
void RealtimeObject::tick(double elapsedSeconds) {}

glm::vec3 RealtimeObject::pos() const {
    // get position from last column of CTM
    // TODO this always works right?
    return {m_ctm[3]};
}

const std::shared_ptr<PrimitiveMesh>& RealtimeObject::mesh() const {
    return m_mesh;
}

const glm::mat4& RealtimeObject::CTM() const {
    return m_ctm;
}

const glm::mat3& RealtimeObject::inverseTransposeCTM() const {
    return m_inverseOfTranspose3x3CTM;
}

const SceneMaterial& RealtimeObject::material() const {
    return m_material;
}

PrimitiveType RealtimeObject::type() const {
    return m_type;
}

bool RealtimeObject::shouldRender() const {
    return m_shouldRender;
}

void RealtimeObject::setShouldRender(bool shouldRender) {
    m_shouldRender = shouldRender;
}

std::shared_ptr<RealtimeScene> RealtimeObject::scene() const {
    return m_scene.lock();
}

void RealtimeObject::queueFree() {
    m_queuedFree = true;
}

bool RealtimeObject::isQueuedFree() const {
    return m_queuedFree;
}

bool RealtimeObject::usesTexture() const {
    return m_material.textureMap.isUsed && m_texture != nullptr;
}

bool RealtimeObject::glTexAllocated() const {
    return m_glTexAllocated;
}

void RealtimeObject::allocateGLTex() {
    if (!usesTexture()) {
        std::cerr << "Cannot allocate texture for object with no texture" << std::endl;
        return;
    }

    glGenTextures(1, &m_glTexID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_glTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_texture->width, m_texture->height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, m_texture->data.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    m_glTexAllocated = true;
}

void RealtimeObject::finish() {
    if (m_glTexAllocated) {
        glDeleteTextures(1, &m_glTexID);
    }
}

GLuint RealtimeObject::glTexID() const {
    return m_glTexID;
}
