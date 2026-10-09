#include "coreEngine.hpp"
#include <iostream>

CoreEngine::CoreEngine(int width,int height,EGLDisplay display,EGLContext context)
  :width(width),height(height),eglDisplay(display),eglContext(context)
{
}

CoreEngine::~CoreEngine(){
}

void CoreEngine::update(float deltaTime){
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
