#include "realtime.h"
#include "meshes/primitivemesh.h"
#include "mainwindow.h"
#include "realtimescene.h"

#include <iostream>
#include "settings.h"
#include "utils/shaderloader.h"
#include "utils/helpers.h"

// ================== Project 5: Lights, Camera

Realtime::Realtime(int w, int h)
        : m_scene(), m_width(w), m_height(h),
          m_param1(settings.shapeParameter1), m_param2(settings.shapeParameter2),
          m_meshes(PrimitiveMesh::initMeshes(settings.shapeParameter1, settings.shapeParameter2))
{
    // m_prev_mouse_pos = glm::vec2(size().width()/2, size().height()/2);
    // setMouseTracking(true);
    // setFocusPolicy(Qt::StrongFocus);

    // m_keyMap[Qt::Key_W]       = false;
    // m_keyMap[Qt::Key_A]       = false;
    // m_keyMap[Qt::Key_S]       = false;
    // m_keyMap[Qt::Key_D]       = false;
    // m_keyMap[Qt::Key_Control] = false;
    // m_keyMap[Qt::Key_Space]   = false;
}

void Realtime::tryInitScene() {
    // we can't init until the user has selected a scene file
    if (settings.sceneFilePath.empty()) {
        return;
    }

    m_scene = RealtimeScene::init(m_width, m_height, settings.sceneFilePath,
                                  settings.nearPlane, settings.farPlane, m_meshes, m_taken_damage);
}

bool Realtime::isInited() const {
    return m_scene != nullptr;
}

void Realtime::finish() {
    // killTimer(m_timer);
    mainWindow.makeCurrent();

    // Students: anything requiring OpenGL calls when the program exits should be done here
    for (auto& [_, mesh] : m_meshes) {
        mesh->deleteBuffers();
    }

    if (isInited()) {
        m_scene->finish();
    }

    glDeleteProgram(m_filterShader);
    glDeleteProgram(m_phongShader);
    glDeleteVertexArrays(1, &m_fullscreen_vao);
    glDeleteBuffers(1, &m_fullscreen_vbo);
    glDeleteTextures(1, &m_fbo_texture);
    glDeleteRenderbuffers(1, &m_fbo_renderbuffer);
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteProgram(m_fogShader);

    mainWindow.doneCurrent();
}

void Realtime::initializeGL() {
    // m_devicePixelRatio = this->devicePixelRatio();

    // m_timer = startTimer(1000/60);
    // m_elapsedTimer.start();
    *m_taken_damage = false;

    // FBO variables
    m_defaultFBO = 0; // TODO
    m_fbo_width = m_width;
    m_fbo_height = m_height;

    // Initializing GL.
    // GLEW (GL Extension Wrangler) provides access to OpenGL functions.
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error while initializing GL: " << glewGetErrorString(err) << std::endl;
    }
    std::cout << "Initialized GL: Version " << glewGetString(GLEW_VERSION) << std::endl;

    // Allows OpenGL to draw objects appropriately on top of one another
    glEnable(GL_DEPTH_TEST);
    // Tells OpenGL to only draw the front face
    glEnable(GL_CULL_FACE);
    // Tells OpenGL how big the screen is
    auto [width, height] = mainWindow.getViewportSize();
    glViewport(0, 0, width, height);

    // Students: anything requiring OpenGL calls when the program starts should be done here
    // Set clear color to black
    glClearColor(0,0,0,1);

    // enable vsync
    glfwSwapInterval(1);

    m_phongShader = ShaderLoader::createShaderProgram("resources/shaders/default.vert", "resources/shaders/default.frag");
    m_filterShader = ShaderLoader::createShaderProgram("resources/shaders/filter.vert", "resources/shaders/filter.frag");
    m_crosshairShader = ShaderLoader::createShaderProgram("resources/shaders/crosshair.vert", "resources/shaders/crosshair.frag");
    m_fogShader = ShaderLoader::createShaderProgram("resources/shaders/fog.vert", "resources/shaders/fog.frag");

    glUseProgram(m_fogShader);
    glUniform1i(glGetUniformLocation(m_fogShader, "scene"), 0); // Bind the scene texture to texture unit 0
    glUseProgram(0);

    // set uniform texture loc for the filter shader TODO is this needed?
    glUseProgram(m_filterShader);
    glUniform1i(glGetUniformLocation(m_filterShader, "myTexture"), 0);
    glUseProgram(0);
    // set uniform texture loc for object textures in phong shader TODO is this needed?
    glUseProgram(m_phongShader);
    glUniform1i(glGetUniformLocation(m_phongShader, "objTexture"), 0);
    glUseProgram(0);

    for (auto& [_, mesh] : m_meshes) {
        // my updateBuffers() function makes sure the mesh is allocated before updating
        mesh->updateBuffers();
    }

    initializeCrosshair();
    initializeFullscreenQuad();
    makeFBO();
}

void Realtime::initializeFullscreenQuad() {
    std::vector<GLfloat> fullscreen_quad_data =
            { //     POSITIONS    //
                    -1.f, 1.f, 0.0f,
                    0.f, 1.f,
                    -1.f, -1.f, 0.0f,
                    0.f, 0.f,
                    1.f, -1.f, 0.0f,
                    1.f, 0.f,
                    1.f, 1.f, 0.0f,
                    1.f, 1.f,
                    -1.f, 1.f, 0.0f,
                    0.f, 1.f,
                    1.f, -1.f, 0.0f,
                    1.f, 0.f
            };

    // Generate and bind a VBO and a VAO for a fullscreen quad
    glGenBuffers(1, &m_fullscreen_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreen_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (fullscreen_quad_data.size() * sizeof(GLfloat)), fullscreen_quad_data.data(),
                 GL_STATIC_DRAW);
    glGenVertexArrays(1, &m_fullscreen_vao);
    glBindVertexArray(m_fullscreen_vao);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    // Unbind the fullscreen quad's VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Realtime::initializeCrosshair()
{
    // Define crosshair lines in NDC
    std::vector<GLfloat> crosshair_data = {
        // Horizontal line
        -0.05f, 0.0f, 0.0f,  // Start of horizontal line
         0.05f, 0.0f, 0.0f,  // End of horizontal line
        // Vertical line
         0.0f, -0.08f, 0.0f, // Start of vertical line
         0.0f,  0.08f, 0.0f  // End of vertical line
    };

    // Generate and bind a VBO and a VAO for the crosshair
    glGenBuffers(1, &m_crosshair_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_crosshair_vbo);
    glBufferData(GL_ARRAY_BUFFER, crosshair_data.size() * sizeof(GLfloat), crosshair_data.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &m_crosshair_vao);
    glBindVertexArray(m_crosshair_vao);

    // Set vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Realtime::makeFBO() {
    // Generate and bind an empty texture, set its min/mag filter interpolation, then unbind
    glGenTextures(1, &m_fbo_texture);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_fbo_width, m_fbo_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Generate and bind a renderbuffer of the right size, set its format, then unbind
    glGenRenderbuffers(1, &m_fbo_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_fbo_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_fbo_width, m_fbo_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Generate and bind an FBO
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Add our texture as a color attachment, and our renderbuffer as a depth+stencil attachment, to our FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_fbo_renderbuffer);

    // Unbind the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
}

void Realtime::paintGL() {
    if (m_queuedBufferUpdate) {
        for (auto& [_, mesh] : m_meshes) {
            mesh->updateBuffers();
        }
        m_queuedBufferUpdate = false;
    }
    // the scene starts uninitialized, we need to check for if the user has selected a scene file yet (i.e. if the scene is initialized)
    if (!isInited()) {
        return;
    }
    // we don't really know when exactly the scene is initialized, so just check if we've passed the shader to
    // the scene every frame--it's cheap
    if (!m_scene->shaderInitialized()) {
        m_scene->initShader(m_phongShader);
        // m_scene->initShader(m_fogShader);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_fbo_width, m_fbo_height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // paintObjects sets and resets the program
    m_scene->paintObjects();
    //paintaCrosshair sets and resets the program


    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (*m_taken_damage == true) {
        damageTaken();
    }

    float distortion_factor = 0.f;
    if (std::chrono::steady_clock::now() > m_damage_end_time) {
        m_damage_filter = false;
    }
    else {
        distortion_factor = std::chrono::duration_cast<std::chrono::milliseconds>(m_damage_end_time - std::chrono::steady_clock::now()).count();
        distortion_factor /= ON_DAMAGE_SCREEN_RED_MS;
    }

    paintScreenTexture(m_fbo_texture, settings.perPixelFilter, settings.kernelBasedFilter, m_damage_filter, distortion_factor);
    // Render the fog effect
    glUseProgram(m_fogShader);

    glm::vec3 fogColor = glm::vec3(0.4f, 0.4f, 0.5f); // Light grayish-blue fog
    float fogStart = 0.001f;
    float fogEnd = 3.0f;

    helpers::passUniformVec3(m_fogShader, "fogColor", fogColor);
    helpers::passUniformFloat(m_fogShader, "fogStart", fogStart);
    helpers::passUniformFloat(m_fogShader, "fogEnd", fogEnd);
    // helpers::passUniformFloat(m_fogShader, "nearPlane", settings.nearPlane);
    // helpers::passUniformFloat(m_fogShader, "farPlane", settings.farPlane);

    // Bind the rendered scene texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
    glUniform1i(glGetUniformLocation(m_fogShader, "scene"), 0);

    // Render a fullscreen quad
    glBindVertexArray(m_fullscreen_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);

    // Render the crosshair
    paintCrosshair();
}

void Realtime::paintScreenTexture(GLuint texture, bool enableInvert, bool enableBoxBlur, bool enableDamageFilter, float distortion_factor) const {
    glUseProgram(m_filterShader);
    helpers::passUniformInt(m_filterShader, "enableInvert", enableInvert);
    helpers::passUniformInt(m_filterShader, "enableBoxBlur", enableBoxBlur);
    helpers::passUniformInt(m_filterShader, "enableDamage", enableDamageFilter);
    helpers::passUniformFloat(m_filterShader, "distortionFactor", distortion_factor);


    // TODO is fbo_width/height correct? or should it be before device pixel ratio?
    helpers::passUniformInt(m_filterShader, "screenWidth", m_fbo_width);
    helpers::passUniformInt(m_filterShader, "screenHeight", m_fbo_height);

    glBindVertexArray(m_fullscreen_vao);
    // Bind "texture" to slot 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Realtime::paintCrosshair()
{
    // Render the crosshair
    glUseProgram(m_crosshairShader);
    glBindVertexArray(m_crosshair_vao);

    // Crosshair doesn't need a model, view, or projection matrix
    glDrawArrays(GL_LINES, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);

}



void Realtime::resizeGL(int w, int h) {
    m_width = w;
    m_height = h;

    // Tells OpenGL how big the screen is
    glViewport(0, 0, w, h);
    m_fbo_width = m_width;
    m_fbo_height = m_height;
    glDeleteTextures(1, &m_fbo_texture);
    glDeleteRenderbuffers(1, &m_fbo_renderbuffer);
    glDeleteFramebuffers(1, &m_fbo);
    makeFBO();

    if (isInited()) {
        m_scene->setDimensions(w, h);
    } else {
        tryInitScene();
    }
}

void Realtime::damageTaken() {
    *m_taken_damage = false;
    m_damage_filter = true;
    m_damage_end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(ON_DAMAGE_SCREEN_RED_MS);
}

void Realtime::keyPressEvent(int key) {
    m_keyMap[key] = true;
    if (isInited()) {
        m_scene->keyPressEvent(key);
    }
}

void Realtime::keyReleaseEvent(int key) {
    m_keyMap[key] = false;
    if (isInited()) {
        m_scene->keyReleaseEvent(key);
    }
}

void Realtime::mousePressEvent(int button) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        m_mouseDown = true;
    }
    if (isInited()) {
        m_scene->mousePressEvent(button);
    }
}

void Realtime::mouseReleaseEvent(int button) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        m_mouseDown = false;
    }
    if (isInited()) {
        m_scene->mouseReleaseEvent(button);
    }
}

void Realtime::mouseMoveEvent(double xpos, double ypos) {
    // currently doing nothing extra program-wide with mouse movement
    if (isInited()) {
        m_scene->mouseMoveEvent(xpos, ypos);
    }
}

void Realtime::timerEvent(double elapsedSeconds) {
    // close window with escape
    if (m_keyMap[GLFW_KEY_ESCAPE]) {
        mainWindow.close();
    }

    // temp filter test code, use 'i' to invert, and 'b' to blur scene TODO remove
    if (m_keyMap[GLFW_KEY_I]) {
        settings.perPixelFilter = !settings.perPixelFilter;
        m_keyMap[GLFW_KEY_I] = false;
    }

    if (m_keyMap[GLFW_KEY_B]) {
        settings.kernelBasedFilter = !settings.kernelBasedFilter;
        m_keyMap[GLFW_KEY_B] = false;
    }
    if (m_keyMap[GLFW_KEY_L]) {
        settings.fogEnabled = !settings.fogEnabled;
        m_keyMap[GLFW_KEY_L] = false;
    }



    // call scene tick
    if (isInited()) {
        m_scene->tick(elapsedSeconds);
    }
}
