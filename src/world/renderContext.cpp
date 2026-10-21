#include "renderContext.hpp"

#include <iostream>

EngineContext::EngineContext(EGLDisplay display,EGLContext context)
  :display(display),previousDisplay(eglGetCurrentDisplay()),
  previousContext(eglGetCurrentContext()),previousDraw(eglGetCurrentSurface(EGL_DRAW)),
  previousRead(eglGetCurrentSurface(EGL_READ)),active(false),changed(false){
  if(display==EGL_NO_DISPLAY||context==EGL_NO_CONTEXT)return;
  changed=previousDisplay!=display||previousContext!=context;
  active=!changed||eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context);
  if(!active)std::cerr<<"[3DEngine] Failed to make EGL context current\n";
}

EngineContext::~EngineContext(){
  if(!active||!changed)return;
  bool restored;
  if(previousDisplay!=EGL_NO_DISPLAY){
    restored=eglMakeCurrent(previousDisplay,previousDraw,previousRead,previousContext);
  }else{
    restored=eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
  }
  if(!restored)std::cerr<<"[3DEngine] Failed to restore EGL context\n";
}

EngineGLState::EngineGLState(){
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&drawFramebuffer);
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&readFramebuffer);
  glGetIntegerv(GL_RENDERBUFFER_BINDING,&renderbuffer);
  glGetIntegerv(GL_ACTIVE_TEXTURE,&activeTexture);
  glGetIntegerv(GL_TEXTURE_BINDING_2D,&activeTextureBinding);
  glActiveTexture(GL_TEXTURE0);
  glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture0Binding);
  glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpackAlignment);
  glActiveTexture(activeTexture);
  glGetIntegerv(GL_CURRENT_PROGRAM,&program);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vertexArray);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&arrayBuffer);
  glGetIntegerv(GL_VIEWPORT,viewport);
  glGetIntegerv(GL_DEPTH_FUNC,&depthFunction);
  glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask);
  glGetBooleanv(GL_COLOR_WRITEMASK,colorMask);
  glGetFloatv(GL_COLOR_CLEAR_VALUE,clearColor);
  glGetFloatv(GL_DEPTH_CLEAR_VALUE,&clearDepth);
  glGetFloatv(GL_DEPTH_RANGE,depthRange);

  for(unsigned int i=0;i<sizeof(capabilities)/sizeof(capabilities[0]);i++){
    enabled[i]=glIsEnabled(capabilities[i]);
    glDisable(capabilities[i]);
  }
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);
  glDepthRangef(0,1);
  glClearDepthf(1);
  glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
}

EngineGLState::~EngineGLState(){
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER,drawFramebuffer);
  glBindFramebuffer(GL_READ_FRAMEBUFFER,readFramebuffer);
  glBindRenderbuffer(GL_RENDERBUFFER,renderbuffer);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,texture0Binding);
  glPixelStorei(GL_UNPACK_ALIGNMENT,unpackAlignment);
  glActiveTexture(activeTexture);
  if(activeTexture!=GL_TEXTURE0)glBindTexture(GL_TEXTURE_2D,activeTextureBinding);
  glUseProgram(program);
  glBindVertexArray(vertexArray);
  glBindBuffer(GL_ARRAY_BUFFER,arrayBuffer);
  glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
  glDepthFunc(depthFunction);
  glDepthMask(depthMask);
  glDepthRangef(depthRange[0],depthRange[1]);
  glClearDepthf(clearDepth);
  glClearColor(clearColor[0],clearColor[1],clearColor[2],clearColor[3]);
  glColorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);
  for(unsigned int i=0;i<sizeof(capabilities)/sizeof(capabilities[0]);i++){
    if(enabled[i])glEnable(capabilities[i]);
    else glDisable(capabilities[i]);
  }
}
