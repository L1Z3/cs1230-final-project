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

    // FBO stuff
    void makeFBO();
    void paintScreenTexture(GLuint texture, bool enableInvert, bool enableBoxBlur) const;
    void initializeFullscreenQuad();

    void initializeCrosshair();
    /// Paints the crosshair
    void paintCrosshair();
    GLuint m_defaultFBO;
    int m_fbo_width;
    int m_fbo_height;
    GLuint m_fullscreen_vao;
    GLuint m_fullscreen_vbo;
    GLuint m_fbo;
    GLuint m_fbo_texture;
    GLuint m_fbo_renderbuffer;

    GLuint m_crosshair_vao;
    GLuint m_crosshair_vbo;

    bool isInited() const;
    void tryInitScene();
    std::shared_ptr<RealtimeScene> m_scene; // empty shared_ptr, for when the scene has not been chosen yet
    std::map<PrimitiveType, std::shared_ptr<PrimitiveMesh>> m_meshes;
    int m_width;
    int m_height;
    int m_param1;
    int m_param2;
    GLuint m_phongShader;
    GLuint m_filterShader;
    GLuint m_crosshairShader;
    bool m_queuedBufferUpdate = false;
};
