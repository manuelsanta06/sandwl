#pragma once

#include "easyShaders.hpp"
#include "engineApi.h"

#include <GLES3/gl3.h>

class CoreEngine{
public:
  CoreEngine(int width,int height);
  ~CoreEngine();

  void init();

  bool render(unsigned int framebuffer,int width,int height,float deltaTime,const EngineInput *input);

  void relaodShaders();

private:
  void renderScene(float deltaTime);

  bool isInitialized=false;
  unsigned int depthRBO;
  int width;
  int height;

  EasyProgram mainProgram;
    
  glm::mat4 model=glm::mat4(1.0f);
  glm::mat4 view =glm::mat4(1.0f);
  glm::mat4 proj =0;

  unsigned int VAO;
  unsigned int VBO;
  unsigned int EBO;
};
