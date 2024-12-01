#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include <optional>
#include <unordered_map>
#include "utils/sceneparser.h"
#include "realtimescene.h"

// TODO physics ticks and input stuff
class Realtime {
public:
    Realtime(int w, int h);
    void finish();                                      // Called on program exit
public:
    // void tick();                               // Called once per tick of m_timer
    void initializeGL();                       // Called once at the start of the program
    void paintGL();                            // Called whenever the OpenGL context changes or by an update() request
    void resizeGL(int width, int height);      // Called when window size changes
    void keyPressEvent(int key);
    void keyReleaseEvent(int key);
    void mousePressEvent(int button);
    void mouseReleaseEvent(int button);
    void mouseMoveEvent(double xpos, double ypos);
    void timerEvent(double elapsedSeconds);
private:
    // Input Related Variables
    bool m_mouseDown = false;                           // Stores state of left mouse button
    glm::dvec2 m_prev_mouse_pos;                         // Stores mouse position
    std::unordered_map<int, bool> m_keyMap;         // Stores whether keys are pressed or not

    // TODO device correction stuff
    // double m_devicePixelRatio;

    bool isInited() const;
    void tryInitScene();
    std::optional<RealtimeScene> m_scene; // optional, for when the scene has not been chosen yet
    std::map<PrimitiveType, std::shared_ptr<PrimitiveMesh>> m_meshes;
    std::optional<int> m_width;
    std::optional<int> m_height;
    int m_param1;
    int m_param2;
    GLuint m_shader;
    bool m_queuedBufferUpdate = false;
};
