#ifndef ENGINE_API_H
#define ENGINE_API_H

#include <stdbool.h>
#include <stdint.h>

#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine3D Engine3D;

Engine3D* engine_create(int width,int height,EGLDisplay display,EGLContext context);
void engine_destroy(Engine3D* engine);

void engine_update(Engine3D* engine,float delta_time);

bool engineRenderFrame(Engine3D* engine,uint32_t* pixel_buffer,int stride);

#ifdef __cplusplus
}
#endif

#endif
