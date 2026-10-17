#include "wlrRenderer.h"

#include <stdlib.h>
#include <drm_fourcc.h>
#include <wlr/render/allocator.h>
#include <wlr/render/drm_format_set.h>
#include <wlr/render/gles2.h>
#include <wlr/render/swapchain.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

struct sandwl_world_output{
  Engine3D *engine;
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;
  struct wlr_swapchain *swapchain;
  struct wlr_scene_buffer *node;
  int width,height;
  bool failed;
};

static struct wlr_swapchain *world_create_swapchain(struct sandwl_world_output *output,int width,int height){
  const struct wlr_drm_format_set *formats=wlr_renderer_get_texture_formats(
    output->renderer,output->allocator->buffer_caps);
  if(!formats)return NULL;

  const uint32_t candidates[]={DRM_FORMAT_XRGB8888,DRM_FORMAT_ARGB8888};
  for(size_t i=0;i<sizeof(candidates)/sizeof(candidates[0]);i++){
    const struct wlr_drm_format *format=wlr_drm_format_set_get(formats,candidates[i]);
    if(!format)continue;

    //sampling support alone does not guarantee a renderable modifier.
    for(int pass=0;pass<2;pass++){
      for(size_t j=0;j<format->len;j++){
        uint64_t modifier=format->modifiers[j];
        bool fallback=modifier==DRM_FORMAT_MOD_LINEAR||modifier==DRM_FORMAT_MOD_INVALID;
        if(fallback!=(pass==1))continue;
        const struct wlr_drm_format target={
          .format=format->format,.len=1,.capacity=1,.modifiers=&modifier
        };
        struct wlr_swapchain *swapchain=wlr_swapchain_create(output->allocator,width,height,&target);
        if(!swapchain)continue;
        struct wlr_buffer *buffer=wlr_swapchain_acquire(swapchain);
        GLuint framebuffer=buffer?wlr_gles2_renderer_get_buffer_fbo(output->renderer,buffer):0;
        if(buffer)wlr_buffer_unlock(buffer);
        if(framebuffer)return swapchain;
        wlr_swapchain_destroy(swapchain);
      }
    }
  }
  return NULL;
}

struct sandwl_world_output *sandwl_world_output_create(Engine3D *engine,
    struct wlr_renderer *renderer,struct wlr_allocator *allocator,struct wlr_scene_tree *parent){
  if(!engine||!wlr_renderer_is_gles2(renderer))return NULL;
  struct sandwl_world_output *output=calloc(1,sizeof(*output));
  if(!output)return NULL;
  output->engine=engine;
  output->renderer=renderer;
  output->allocator=allocator;
  output->node=wlr_scene_buffer_create(parent,NULL);
  if(!output->node){
    free(output);
    return NULL;
  }
  return output;
}

void sandwl_world_output_destroy(struct sandwl_world_output *output){
  if(!output)return;
  wlr_scene_node_destroy(&output->node->node);
  wlr_swapchain_destroy(output->swapchain);
  free(output);
}

bool sandwl_world_output_render(struct sandwl_world_output *output,int width,int height,
    const struct wlr_box *box,float delta_time){
  if(!output||width<=0||height<=0||box->width<=0||box->height<=0)return false;
  wlr_scene_node_set_position(&output->node->node,box->x,box->y);
  wlr_scene_buffer_set_dest_size(output->node,box->width,box->height);

  if(width!=output->width||height!=output->height){
    output->width=width;
    output->height=height;
    output->failed=false;
    struct wlr_swapchain *swapchain=world_create_swapchain(output,width,height);
    if(!swapchain){
      output->failed=true;
      wlr_log(WLR_ERROR,"Failed to allocate a renderable 3D buffer (%dx%d)",width,height);
      return false;
    }
    wlr_swapchain_destroy(output->swapchain);
    output->swapchain=swapchain;
  }
  if(output->failed||!output->swapchain)return false;

  struct wlr_buffer *buffer=wlr_swapchain_acquire(output->swapchain);
  if(!buffer){
    wlr_log(WLR_ERROR,"Failed to acquire a 3D buffer");
    return false;
  }
  GLuint framebuffer=wlr_gles2_renderer_get_buffer_fbo(output->renderer,buffer);
  bool rendered=framebuffer&&engine_render(output->engine,framebuffer,width,height,delta_time);
  if(rendered){
    wlr_scene_buffer_set_buffer_with_damage(output->node,buffer,NULL);
  }else{
    wlr_log(WLR_ERROR,"Failed to render the 3D background");
  }
  wlr_buffer_unlock(buffer);
  return rendered;
}
