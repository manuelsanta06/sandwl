#include "engineApi.h"
#include "coreEngine.hpp"

Engine3D* engine_create(int width,int height,EGLDisplay display,EGLContext context){
  CoreEngine* engine=new CoreEngine(width,height,display,context);
  return reinterpret_cast<Engine3D*>(engine);
}

void engine_destroy(Engine3D* engine_ptr){
  if(!engine_ptr)return;
  CoreEngine* engine=reinterpret_cast<CoreEngine*>(engine_ptr);
  delete engine;
}

void engine_update(Engine3D* engine_ptr,float delta_time){
  if(!engine_ptr)return;
  CoreEngine* engine=reinterpret_cast<CoreEngine*>(engine_ptr);
  engine->update(delta_time);
}
