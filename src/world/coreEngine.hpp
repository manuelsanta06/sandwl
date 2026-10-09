#pragma once

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <cstdint>

class CoreEngine{
public:
    CoreEngine(int width,int height,EGLDisplay display,EGLContext context);
    ~CoreEngine();

    void update(float deltaTime);

private:
    int width;
    int height;
    
    EGLDisplay eglDisplay;
    EGLContext eglContext;
};
