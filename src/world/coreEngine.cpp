#include "easyShaders.hpp"
#include "coreEngine.hpp"

#include <GLES3/gl3.h>
#include <GL/gl.h>

CoreEngine::CoreEngine(int width,int height,EGLDisplay display,EGLContext context)
  :width(width),height(height),eglDisplay(display),eglContext(context)
{
}

void CoreEngine::init(){
  float vertices[]={
    0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
    -0.5f,-0.5f, 0.0f,  0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, 0.0f,  1.0f, 0.0f, 1.0f
  };
  unsigned int indices[]={
    0, 1, 3,
    1, 2, 3
  };

  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1,&VBO);
  glBindBuffer(GL_ARRAY_BUFFER,VBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);

  glGenBuffers(1,&EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);

  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  mainProgram=EasyProgram("src/world/shaders/basic.vert","src/world/shaders/basic.frag");
}

CoreEngine::~CoreEngine(){}

void CoreEngine::update(float deltaTime){
  if(!isInitialized){
    isInitialized=true;
    this->init();
  }
  glViewport(0,0,width,height);

  mainProgram.use();
  glBindVertexArray(VAO);

  glClearColor(0.2f, 0.1f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // glDrawElements(GL_LINE_LOOP, 6, GL_UNSIGNED_INT, 0);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
  glUseProgram(0);
}
