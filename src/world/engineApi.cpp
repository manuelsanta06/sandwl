#include "engineApi.h"
#include "coreEngine.hpp"
#include "renderContext.hpp"

#include <cstdio>
#include <iostream>
#include <new>

struct Engine3D{
  EGLDisplay display;
  EGLContext context;
  CoreEngine core;

  Engine3D(EGLDisplay display,EGLContext context)
    :display(display),context(context),core(0,0){}
};

Engine3D* engine_create(EGLDisplay display,EGLContext context){
  EngineContext current(display,context);
  if(!current)return nullptr;
  const char *version=reinterpret_cast<const char*>(glGetString(GL_VERSION));
  int major=0,minor=0;
  if(!version||std::sscanf(version,"OpenGL ES %d.%d",&major,&minor)!=2||
      major<3||(major==3&&minor<2)){
    std::cerr<<"[3DEngine] OpenGL ES 3.2 is required by the current shaders\n";
    return nullptr;
  }
  return new(std::nothrow) Engine3D(display,context);
}

void engine_destroy(Engine3D* engine){
  if(!engine)return;
  EngineContext current(engine->display,engine->context);
  if(!current)return;
  delete engine;
}

bool engine_render(Engine3D* engine,uint32_t framebuffer,int width,int height,
    float delta_time,const struct EngineInput *input){
  if(!engine||!framebuffer||width<=0||height<=0)return false;
  EngineContext current(engine->display,engine->context);
  if(!current)return false;
  EngineGLState state;
  return engine->core.render(framebuffer,width,height,delta_time,input);
}

void reload_shaders(Engine3D* engine){
  if(!engine)return;
  EngineContext current(engine->display,engine->context);
  if(!current)return;
  EngineGLState state;
  engine->core.relaodShaders();
}
