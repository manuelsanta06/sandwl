#pragma once

#include <stdbool.h>

#include "engineApi.h"

struct wlr_allocator;
struct wlr_box;
struct wlr_renderer;
struct wlr_scene_tree;
struct sandwl_world_output;

struct sandwl_world_output *sandwl_world_output_create(Engine3D *engine,
  struct wlr_renderer *renderer,struct wlr_allocator *allocator,struct wlr_scene_tree *parent);
void sandwl_world_output_destroy(struct sandwl_world_output *output);
bool sandwl_world_output_render(struct sandwl_world_output *output,int width,int height,
  const struct wlr_box *box,float delta_time,const struct EngineInput *input);
