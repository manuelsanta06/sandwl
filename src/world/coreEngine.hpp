#pragma once

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <cstdint>

class CoreEngine{
public:
    CoreEngine(int width,int height,EGLDisplay display,EGLContext context);
    ~CoreEngine();

    void update(float deltaTime);
    
    uint32_t getTextureID()const{return colorTexture;}

private:
    int width;
    int height;
    
    EGLDisplay eglDisplay;
    EGLContext eglContext;

    GLuint fbo;
    GLuint colorTexture;

    bool makeCurrent();
    void initFBO();
};
