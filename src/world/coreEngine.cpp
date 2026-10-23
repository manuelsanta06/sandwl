#include "easyShaders.hpp"
#include "coreEngine.hpp"
#include "inputData.hpp"
#include "engineApi.h"
#include "camera.hpp"
#include "textureLoader.hpp"

#include <GLES3/gl3.h>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

CoreEngine::CoreEngine(int width,int height)
  :mainCamera(Camera(glm::vec3(0,0,3))),width(width),height(height)
{}

void CoreEngine::relaodShaders(){
  mainProgram.load("src/world/shaders/basic.vert","src/world/shaders/basic.frag");
  lightProgram.load("src/world/shaders/light.vert","src/world/shaders/light.frag");
}

void CoreEngine::init(){
  glEnable(GL_DEPTH_TEST);
  glGenRenderbuffers(1,&depthRBO);
  glBindRenderbuffer(GL_RENDERBUFFER,depthRBO);
  glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);
  glBindRenderbuffer(GL_RENDERBUFFER,0);

  glGenVertexArrays(1,&VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1,&VBO);
  glBindBuffer(GL_ARRAY_BUFFER,VBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(verticesCube),verticesCube,GL_STATIC_DRAW);

  glGenBuffers(1,&EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indexPlane),indexPlane,GL_STATIC_DRAW);

  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
  glEnableVertexAttribArray(2);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  glGenVertexArrays(1,&lightVAO);
  glBindVertexArray(lightVAO);
  glBindBuffer(GL_ARRAY_BUFFER,VBO);
  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
  glEnableVertexAttribArray(0);

  mainProgram.load("src/world/shaders/basic.vert","src/world/shaders/basic.frag");
  lightProgram.load("src/world/shaders/light.vert","src/world/shaders/light.frag");
  diffuseMap=loadTexture("src/world/textures/metalBoxDiffuser.png");
  specularMap=loadTexture("src/world/textures/metalBoxSpecular.png");
}

CoreEngine::~CoreEngine(){
  if(!isInitialized)return;
  glDeleteRenderbuffers(1,&depthRBO);
  glDeleteVertexArrays(1,&VAO);
  glDeleteVertexArrays(1,&lightVAO);
  glDeleteBuffers(1,&VBO);
  glDeleteBuffers(1,&EBO);
  if(diffuseMap)glDeleteTextures(1,&diffuseMap);
}

bool CoreEngine::render(unsigned int framebuffer,int targetWidth,int targetHeight,float deltaTime,const EngineInput *input){
  if(!framebuffer||targetWidth<=0||targetHeight<=0)return false;
  glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);

  //wlroots owns this FBO; only borrow its empty depth attachment.
  GLint depthType;
  glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,
    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,&depthType);
  if(depthType!=GL_NONE)return false;

  bool resized=width!=targetWidth||height!=targetHeight;
  width=targetWidth;
  height=targetHeight;
  if(!isInitialized){
    this->init();
    isInitialized=true;
  }else if(resized){
    glBindRenderbuffer(GL_RENDERBUFFER,depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);
  }
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depthRBO);
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,0);
    std::cerr<<"[3DEngine] Render target is incomplete\n";
    return false;
  }

  renderScene(deltaTime,input);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,0);
  glFlush();
  return true;
}

void CoreEngine::renderScene(float deltaTime,const EngineInput *input){
  mainCamera.processInput(input,deltaTime);
  static float degsCubes=0;
  static float degsLight=0;
  glm::vec3 lightPos(3.0f,0.0f,-3.0f);
  // degsCubes+=45*deltaTime;
  if(degsCubes>=360)degsCubes-=360;
  degsLight+=10*deltaTime;
  if(degsLight>=360)degsLight-=360;

  //clears
  glViewport(0,0,width,height);
  glClearColor(0.2f,0.1f,0.3f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  mainProgram.use();
  glBindVertexArray(VAO);

  //shaders uniforms
  float degFov=45.0f;
  proj =glm::perspective(glm::radians(degFov),(float)width/(float)height,0.1f,100.0f);
  proj[1][1]*=-1;

  glm::mat4 view=mainCamera.GetViewMatrix();
  mainProgram.setUniform("view",view);
  mainProgram.setUniform("projection",proj);

  mainProgram.setUniform("material.diffuse",0);
  mainProgram.setUniform("material.specular",1);
  mainProgram.setUniform("material.shininess",32.0f);
  glm::vec3 currentLightPos=glm::vec3(
      (glm::rotate(glm::mat4(1.0f),glm::radians(degsLight),glm::vec3(0.0f,0.0f,1.0f)))
      *glm::vec4(lightPos,1.0f));

  mainProgram.setUniform("pointLight.position",currentLightPos);
  mainProgram.setUniform("pointLight.ambient",0.2f,0.2f,0.2f);
  mainProgram.setUniform("pointLight.diffuse",0.5f,0.5f,0.5f);
  mainProgram.setUniform("pointLight.specular",1.0f,1.0f,1.0f);
  mainProgram.setUniform("pointLight.constant",1.0f);
  mainProgram.setUniform("pointLight.linear",0.09f);
  mainProgram.setUniform("pointLight.quadratic",0.032f);

  mainProgram.setUniform("spotLight.position",mainCamera.m_position);
  // mainProgram.setUniform("spotLight.position",currentLightPos);
  mainProgram.setUniform("spotLight.ambient",0.2f,0.2f,0.2f);
  mainProgram.setUniform("spotLight.diffuse",0.5f,0.5f,0.5f);
  mainProgram.setUniform("spotLight.specular",1.0f,1.0f,1.0f);
  mainProgram.setUniform("spotLight.direction",mainCamera.m_front);
  mainProgram.setUniform("spotLight.cutOff",glm::cos(glm::radians(7.0f)));
  mainProgram.setUniform("spotLight.outerCutOff",glm::cos(glm::radians(12.0f)));

  mainProgram.setUniform("viewPos",mainCamera.m_position);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,diffuseMap);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D,specularMap);

  //draws
  // glDrawElements(GL_LINE_LOOP,6,GL_UNSIGNED_INT,0);
  // glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
  for(short i=0;i<10;i++){
    model=glm::translate(glm::mat4(1.0f),cubePositions[i]);
    model=glm::rotate(model,glm::radians(degsCubes*(i==0?1:i)),glm::vec3(1.0f,1.0f,1.0f));
    mainProgram.setUniform("model",model);
    glDrawArrays(GL_TRIANGLES,0,36);
  }

  lightProgram.use();
  lightProgram.setUniform("view",view);
  lightProgram.setUniform("projection",proj);
  model=glm::mat4(1.0f);
  model=glm::translate(model,currentLightPos);
  model=glm::scale(model,glm::vec3(0.2f));
  lightProgram.setUniform("model",model);
  glBindVertexArray(lightVAO);
  glDrawArrays(GL_TRIANGLES,0,36);

  glBindVertexArray(0);
  glUseProgram(0);
}
