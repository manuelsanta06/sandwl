#pragma once

#include <EGL/egl.h>
#include <GLES3/gl3.h>

class EngineContext{
public:
  EngineContext(EGLDisplay display,EGLContext context);
  ~EngineContext();
  explicit operator bool()const{return active;}

  EngineContext(const EngineContext&)=delete;
  EngineContext& operator=(const EngineContext&)=delete;

private:
  EGLDisplay display;
  EGLDisplay previousDisplay;
  EGLContext previousContext;
  EGLSurface previousDraw;
  EGLSurface previousRead;
  bool active;
  bool changed;
};

class EngineGLState{
public:
  EngineGLState();
  ~EngineGLState();

  EngineGLState(const EngineGLState&)=delete;
  EngineGLState& operator=(const EngineGLState&)=delete;

private:
  static constexpr GLenum capabilities[]={
    GL_BLEND,GL_CULL_FACE,GL_DEPTH_TEST,GL_SCISSOR_TEST,GL_STENCIL_TEST,
    GL_RASTERIZER_DISCARD,GL_POLYGON_OFFSET_FILL,
    GL_SAMPLE_ALPHA_TO_COVERAGE,GL_SAMPLE_COVERAGE
  };
  GLboolean enabled[sizeof(capabilities)/sizeof(capabilities[0])];
  GLint drawFramebuffer,readFramebuffer,renderbuffer;
  GLint activeTexture,activeTextureBinding,texture0Binding,unpackAlignment;
  GLint program,vertexArray,arrayBuffer;
  GLint viewport[4];
  GLint depthFunction;
  GLboolean depthMask,colorMask[4];
  GLfloat clearColor[4],clearDepth,depthRange[2];
};
