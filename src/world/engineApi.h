#ifndef ENGINE_API_H
#define ENGINE_API_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine3D Engine3D;

Engine3D* engineCreate(int width, int height);
void engineDestroy(Engine3D* engine);

void engineUpdate(Engine3D* engine, float delta_time);

bool engineRenderFrame(Engine3D* engine, uint32_t* pixel_buffer, int stride);

#ifdef __cplusplus
}
#endif

#endif
