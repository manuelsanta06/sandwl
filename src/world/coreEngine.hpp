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

  void relaodShaders();

private:
  bool isInitialized=false;
  unsigned int depthRBO;
  int width;
  int height;

  EasyProgram mainProgram;
    
  EGLDisplay eglDisplay;
  EGLContext eglContext;

  glm::mat4 model=glm::mat4(1.0f);
  glm::mat4 view =glm::mat4(1.0f);
  glm::mat4 proj =0;

  unsigned int VAO;
  unsigned int VBO;
  unsigned int EBO;
};
