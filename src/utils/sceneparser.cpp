#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <iostream>

void traverseSceneGraph(SceneNode* node,
                        const glm::mat4& curCTM,
                        std::vector<RenderShapeData>& shapes,
                        std::vector<SceneLightData>& lights) {
    if (node == nullptr)
        return;

    glm::mat4 CTM = curCTM;
    // apply transformations to the current CTM
    for (SceneTransformation* transformation: node->transformations) {
        switch (transformation->type) {
            case TransformationType::TRANSFORMATION_TRANSLATE:
                CTM = CTM * glm::translate(transformation->translate);
                break;
            case TransformationType::TRANSFORMATION_SCALE:
                CTM = CTM * glm::scale(transformation->scale);
                break;
            case TransformationType::TRANSFORMATION_ROTATE:
                CTM = CTM * glm::rotate(transformation->angle, transformation->rotate);
                break;
            case TransformationType::TRANSFORMATION_MATRIX:
                CTM = CTM * transformation->matrix;
                break;
        }
    }

    // add the shapes and the current CTM to the shapes vector
    for (ScenePrimitive* primitive: node->primitives) {
        shapes.push_back({*primitive, CTM});
    }

    // apply the transformation to the lights and add them to the lights vector
    for (SceneLight* light: node->lights) {
        glm::vec4 dir;
        glm::vec4 pos;
        switch (light->type) {
            case LightType::LIGHT_POINT:
                pos = CTM * glm::vec4(0.f, 0.f, 0.f, 1.f);
                break;
            case LightType::LIGHT_DIRECTIONAL:
                dir = CTM * glm::vec4(light->dir);
                break;
            case LightType::LIGHT_SPOT:
                pos = CTM * glm::vec4(0.f, 0.f, 0.f, 1.f);
                dir = CTM * glm::vec4(light->dir);
                break;
        }
        lights.push_back({light->id, light->type, light->color, light->function, pos, dir, light->penumbra, light->angle, light->width, light->height});
    }

    // recur on all children
    for (SceneNode* child: node->children) {
        traverseSceneGraph(child, CTM, shapes, lights);
    }
}

bool SceneParser::parse(std::string filepath, RenderData &renderData) {
    ScenefileReader fileReader = ScenefileReader(filepath);
    bool success = fileReader.readJSON();
    if (!success) {
        return false;
    }

    renderData.globalData = fileReader.getGlobalData();
    renderData.cameraData = fileReader.getCameraData();
    renderData.shapes.clear();
    renderData.lights.clear();

    SceneNode* rootNode = fileReader.getRootNode();
    // start process with identity CTM, and the (currently empty) shapes and lights vectors
    traverseSceneGraph(rootNode, glm::identity<glm::mat4>(), renderData.shapes, renderData.lights);

    return true;
}
