#include "easyShaders.hpp"
#include "coreEngine.hpp"
#import "inputData.hpp"

#include <GLES3/gl3.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

CoreEngine::CoreEngine(int width,int height,EGLDisplay display,EGLContext context)
  :width(width),height(height),eglDisplay(display),eglContext(context)
{}

void CoreEngine::relaodShaders(){
  mainProgram.load("src/world/shaders/basic.vert","src/world/shaders/basic.frag");
}

void CoreEngine::init(){
  glEnable(GL_DEPTH_TEST);
  glGenRenderbuffers(1,&depthRBO);
  glBindRenderbuffer(GL_RENDERBUFFER,depthRBO);
  glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);
  glBindRenderbuffer(GL_RENDERBUFFER,0);

  model=glm::rotate(glm::mat4(1.0f),glm::radians(-55.0f),glm::vec3(1.0f,0.0f,0.0f));
  view =glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,0.0f,-3.0f));
  float degFov=45.0f;
  proj =glm::perspective(glm::radians(degFov),(float)width/(float)height,0.1f,100.0f);
  proj[1][1]*=-1;

  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1,&VBO);
  glBindBuffer(GL_ARRAY_BUFFER,VBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(verticesCube),verticesCube,GL_STATIC_DRAW);

  glGenBuffers(1,&EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indexPlane),indexPlane,GL_STATIC_DRAW);

  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  mainProgram.load("src/world/shaders/basic.vert","src/world/shaders/basic.frag");
}

CoreEngine::~CoreEngine(){}

void CoreEngine::update(float deltaTime){
  if(!isInitialized){
    isInitialized=true;
    this->init();
  }
  //set the enviroment up
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
  mainProgram.use();
  glBindVertexArray(VAO);

  //clears
  glViewport(0,0,width,height);
  glClearColor(0.2f, 0.1f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  //shaders uniforms
  mainProgram.setUniform("view",view);
  mainProgram.setUniform("projection",proj);

  //draws
  // glDrawElements(GL_LINE_LOOP, 6, GL_UNSIGNED_INT, 0);
  // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  static float degs=0;
  degs+=45*deltaTime;
  if(degs>=360)degs-=360;
  for(short i=0;i<10;i++){
    model=glm::translate(glm::mat4(1.0f),cubePositions[i]);
    model=glm::rotate(model,glm::radians(degs*(i==0?1:i)),glm::vec3(1.0f,1.0f,1.0f));
    mainProgram.setUniform("model",model);
    glDrawArrays(GL_TRIANGLES,0,36);
  }

  //restore original enviroment for wlroots
  glBindVertexArray(0);
  glUseProgram(0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,0);
}
