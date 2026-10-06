#include "engineApi.h"
#include <cstdint>

class CoreEngine{
  public:
  CoreEngine(int width,int height){
  }

  ~CoreEngine(){
  }

  void update(float delta_time){
  }

  bool renderFrame(uint32_t* pixel_buffer,int stride){
    return true; 
  }
};

Engine3D* engine_create(int width,int height){
  CoreEngine* engine=new CoreEngine(width,height);
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

bool engine_render_frame(Engine3D* engine_ptr,uint32_t* pixel_buffer,int stride){
  if(!engine_ptr)return false;
  CoreEngine* engine=reinterpret_cast<CoreEngine*>(engine_ptr);
  return engine->renderFrame(pixel_buffer,stride);
}
