#ifndef ENGINE_API_H
#define ENGINE_API_H

#include <stdbool.h>
#include <stdint.h>

#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine3D Engine3D;

struct EngineInput{
  float forward;
  float right;
  float yaw_delta;
  float pitch_delta;
};

Engine3D* engine_create(EGLDisplay display,EGLContext context);
void engine_destroy(Engine3D* engine);

// The FBO must belong to the engine's EGL context. delta_time is in seconds.
// Call outside wlroots render passes; EGL and the modified GL state are restored.
bool engine_render(Engine3D* engine,uint32_t framebuffer,int width,int height,
  float delta_time,const struct EngineInput *input);

void reload_shaders(Engine3D* engine);

#ifdef __cplusplus
}
#endif

#endif
