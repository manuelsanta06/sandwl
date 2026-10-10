#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "coreEngine.hpp"

std::string readShaderFile(const std::string& filePath){
  std::ifstream file(filePath);
  if(!file.is_open()){
    std::cerr<<"[3DEngine] Error: no such shader" << filePath << std::endl;
    return "";
  }

  std::stringstream buffer;
  buffer<<file.rdbuf();
  file.close();

  return buffer.str();
}

bool loadShader(unsigned int &id,int type,const std::string& filePath){
  id=glCreateShader(type);
  std::string rawCode=readShaderFile(filePath);
  const char* castedCode=rawCode.c_str();

  glShaderSource(id,1,&castedCode,NULL);
  glCompileShader(id);

  int  success;
  char infoLog[512];
  glGetShaderiv(id,GL_COMPILE_STATUS,&success);
  if(!success){
    glGetShaderInfoLog(id,512,NULL,infoLog);
    std::cerr<<"[3Dengine] ERROR::SHADER::COMPILATION_FAILED file"<<filePath<<"\n"<<infoLog<<std::endl;
    return false;
  }
  return true;
}

CoreEngine::CoreEngine(int width,int height,EGLDisplay display,EGLContext context)
  :width(width),height(height),eglDisplay(display),eglContext(context)
{
}

void CoreEngine::init(){
  float vertices[]={
    -0.5f,-0.5f,0.0f,
     0.5f,-0.5f,0.0f,
     0.0f, 0.5f,0.0f
  };

  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1,&VBO);
  glBindBuffer(GL_ARRAY_BUFFER,VBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);

  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  unsigned int Sfragment;
  unsigned int Svertex;
  loadShader(Svertex,GL_VERTEX_SHADER,"src/world/shaders/basic.vert");
  loadShader(Sfragment,GL_FRAGMENT_SHADER,"src/world/shaders/basic.frag");

  shaderProgram=glCreateProgram();
  glAttachShader(shaderProgram,Svertex);
  glAttachShader(shaderProgram,Sfragment);
  glLinkProgram(shaderProgram);

  glDeleteShader(Svertex);
  glDeleteShader(Sfragment);
}

CoreEngine::~CoreEngine(){}

void CoreEngine::update(float deltaTime){
  if(!isInitialized){
    isInitialized=true;
    this->init();
  }
  glViewport(0,0,width,height);

  glClearColor(0.2f, 0.1f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(shaderProgram);
  glBindVertexArray(VAO);

  glDrawArrays(GL_TRIANGLES,0,3);

  glBindVertexArray(0);
  glUseProgram(0);

  // static int flashs=0;
  // if(flashs>120)
  //   glClearColor(0.2f,0.1f,0.3f,1.0f);
  // else
  //   glClearColor(1.f,1.f,1.f,1.f);
  // glClear(GL_COLOR_BUFFER_BIT);
  // if(++flashs>=250)flashs=0;

  // glBindFramebuffer(GL_FRAMEBUFFER,0);
}
