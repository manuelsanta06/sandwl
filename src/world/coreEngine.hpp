#pragma once

#include "easyShaders.hpp"

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

  EasyProgram mainProgram;

  unsigned int VAO;
  unsigned int VBO;
  unsigned int EBO;
    
  EGLDisplay eglDisplay;
  EGLContext eglContext;
};
