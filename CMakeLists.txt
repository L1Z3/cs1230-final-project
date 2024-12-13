cmake_minimum_required(VERSION 3.16)

# Set the project name and languages
project(cs1230_final_lights_camera_explosion LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# no optimizations for debug build
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

# Include the current directory for header files
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_PREFIX_PATH "C:/Users/bcher/Desktop/CS1230/glfw-3.4.bin.WIN64")

# Allows you to include files from within those directories, without prefixing their filepaths
include_directories(src)

# enable glm swizzling
add_definitions(-DGLM_FORCE_SWIZZLE)

# Specifies .cpp and .h files to be passed to the compiler
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/main.cpp
    src/utils/shaderloader.h
    src/realtime.cpp
    src/realtime.h
    src/utils/sceneparser.cpp
    src/utils/sceneparser.h
    src/utils/scenedata.h
    src/utils/scenefilereader.cpp
    src/utils/scenefilereader.h
    src/settings.cpp
    src/settings.h
    src/mainwindow.cpp
    src/mainwindow.h
    src/objects/realtimeobject.cpp
    src/objects/realtimeobject.h
    src/meshes/primitivemesh.cpp
    src/meshes/primitivemesh.h
    src/meshes/conemesh.cpp
    src/meshes/conemesh.h
    src/meshes/cylindermesh.cpp
    src/meshes/cylindermesh.h
    src/meshes/cubemesh.cpp
    src/meshes/cubemesh.h
    src/meshes/spheremesh.cpp
    src/meshes/spheremesh.h
    src/realtimescene.cpp
    src/realtimescene.h
    src/camera.cpp
    src/camera.h
    src/aabb.h
    src/objects/staticobject.cpp
    src/objects/staticobject.h
    src/objects/collisionobject.cpp
    src/objects/collisionobject.h
    src/objects/collisionobject.cpp
    src/objects/collisionobject.h
    src/objects/staticobject.cpp
    src/objects/staticobject.h
    src/aabb.cpp
    src/objects/playerobject.cpp
    src/objects/playerobject.h
    src/objects/enemyobject.cpp
    src/objects/enemyobject.h
    src/objects/projectileobject.cpp
    src/objects/projectileobject.h
    src/utils/helpers.h
    src/utils/helpers.cpp
    src/material_constants/enemy_materials.cpp
    src/material_constants/enemy_materials.h
    src/objects/ncprojectileobject.cpp
    src/objects/ncprojectileobject.h
        src/utils/imagereader.cpp
        src/utils/imagereader.h
        src/utils/rgba.h
        src/utils/stb_image.h
        src/meshes/skymesh.cpp
        src/meshes/skymesh.h
        src/objects/skyboxobject.h
        src/objects/skyboxobject.cpp
        resources/shaders/fog.frag
        resources/shaders/fog.vert


)

# GLM: this creates its library and allows you to `#include "glm/..."`
add_subdirectory(glm)

# GLEW: this creates its library and allows you to `#include "GL/glew.h"`
add_library(StaticGLEW STATIC glew/src/glew.c)
include_directories(${PROJECT_NAME} PRIVATE glew/include)

# for fetching the source of external libraries
include(FetchContent)

# nlohmann_json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

# find OpenGL
find_package(OpenGL REQUIRED)

# I LOVE WINDOWS
if (MINGW)
  FetchContent_Declare(glfw URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip)
  FetchContent_MakeAvailable(glfw)
else()
  find_package(glfw3 3.4 REQUIRED)
endif()

# Include GLFW directories
include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})
add_definitions(${GLFW_CFLAGS_OTHER})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    StaticGLEW
    nlohmann_json::nlohmann_json
)

# GLEW: this provides support for Windows (including 64-bit)
if (WIN32)
  add_compile_definitions(GLEW_STATIC)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    opengl32
    glu32
  )
endif()

# Set this flag to silence warnings on Windows
if (MSVC OR MSYS OR MINGW)
  set(CMAKE_CXX_FLAGS "-Wno-volatile")
endif()
# Set this flag to silence warnings on MacOS
if (APPLE)
  set(CMAKE_CXX_FLAGS "-Wno-deprecated-volatile")
endif()
