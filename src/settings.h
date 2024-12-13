#pragma once

#include <string>
using namespace std;
struct Settings {
    std::string sceneFilePath;
    int shapeParameter1 = 1;
    int shapeParameter2 = 1;
    float nearPlane = 1;
    float farPlane = 1;
    bool perPixelFilter = false;
    bool kernelBasedFilter = false;
    bool fogEnabled=true;
};


// The global Settings object
extern Settings settings;
