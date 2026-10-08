#include "coreEngine.hpp"
#include <iostream>

CoreEngine::CoreEngine(int width,int height,EGLDisplay display,EGLContext context)
  :width(width),height(height),eglDisplay(display),eglContext(context),fbo(0),colorTexture(0)
{
  initFBO();
}

CoreEngine::~CoreEngine(){
  if(makeCurrent()){
    glDeleteFramebuffers(1,&fbo);
    glDeleteTextures(1,&colorTexture);
  }
}

bool CoreEngine::makeCurrent(){
  if(eglMakeCurrent(eglDisplay,EGL_NO_SURFACE,EGL_NO_SURFACE,eglContext)==EGL_FALSE){
    std::cerr<<"[3DEngine] Error: could not activate EGL context."<< std::endl;
    return false;
  }
  return true;
}

void CoreEngine::initFBO(){
  if(!makeCurrent())return;

  glGenTextures(1,&colorTexture);
  glBindTexture(GL_TEXTURE_2D,colorTexture);

  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);

  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  glGenFramebuffers(1,&fbo);
  glBindFramebuffer(GL_FRAMEBUFFER,fbo);

  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,colorTexture,0);

  if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){
    std::cerr<<"[3DEngine] Error: FBO incompleto."<<std::endl;
  }else{
    std::cout<<"[3DEngine] FBO and texture("<<width<<"x"<<height<<") initialized."<<std::endl;
  }

  glBindFramebuffer(GL_FRAMEBUFFER,0);
}

void CoreEngine::update(float deltaTime){
  if(!makeCurrent())return;
  glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glViewport(0,0,width,height);

  static int flashs=0;
  if(flashs>120)
    glClearColor(0.2f,0.1f,0.3f,1.0f);
  else
    glClearColor(1.f,1.f,1.f,1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  if(++flashs>=250)flashs=0;

  glBindFramebuffer(GL_FRAMEBUFFER,0);
}
