#pragma once

#include "dataAbstractions.hpp"
#include "easyShaders.hpp"
#include "engineApi.h"
#include "camera.hpp"

#include <GLES3/gl3.h>

class CoreEngine{
public:
  CoreEngine(int width,int height);
  ~CoreEngine();

  void init();

  bool render(unsigned int framebuffer,int width,int height,float deltaTime,const EngineInput *input);

  void relaodShaders();

private:
  void renderScene(float deltaTime,const EngineInput *input);

  Camera mainCamera;

  bool isInitialized=false;
  unsigned int depthRBO;
  int width;
  int height;

  EasyProgram mainProgram;
  EasyProgram lightProgram;
  
  unsigned int lightVAO;
  unsigned int VAO;
  unsigned int VBO;
  unsigned int EBO;

  unsigned int diffuseMap=0;
  unsigned int specularMap=0;

  Model backpack=0;
  Model elephant=0;
};
