#pragma once

#include <EGL/egl.h>
#include <GLES3/gl3.h>

class CoreEngine{
public:
  CoreEngine(int width,int height,EGLDisplay display,EGLContext context);
  ~CoreEngine();

  void init();

  void update(float deltaTime);

private:
  bool isInitialized=false;
  int width;
  int height;

  unsigned int shaderProgram;

  unsigned int VAO;
  unsigned int VBO;
    
  EGLDisplay eglDisplay;
  EGLContext eglContext;
};
