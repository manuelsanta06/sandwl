#include "textureLoader.hpp"

#include <GLES3/gl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

#include <algorithm>
#include <iostream>
#include <limits>

static unsigned int uploadTexture(stbi_uc *pixels,int width,int height){
  size_t rowStride=(size_t)width*4;
  for(int y=0;y<height/2;y++){
    auto top=pixels+y*rowStride;
    auto bottom=pixels+(height-1-y)*rowStride;
    std::swap_ranges(top,top+rowStride,bottom);
  }

  glActiveTexture(GL_TEXTURE0);

  GLuint texture=0;
  glGenTextures(1,&texture);
  glBindTexture(GL_TEXTURE_2D,texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,
    GL_UNSIGNED_BYTE,pixels);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  return texture;
}

unsigned int loadTextureFile(const char *path){
  int width,height,channels;
  stbi_uc *pixels=stbi_load(path,&width,&height,&channels,STBI_rgb_alpha);
  if(!pixels){
    const char *error=stbi_failure_reason();
    std::cerr<<"[3DEngine] Failed to load texture "<<path<<": "
      <<(error?error:"unknown error")<<std::endl;
    return 0;
  }
  unsigned int texture=uploadTexture(pixels,width,height);
  stbi_image_free(pixels);
  return texture;
}

unsigned int loadTextureMemory(const unsigned char *data,size_t size){
  if(!data||!size||size>std::numeric_limits<int>::max())return 0;
  int width,height,channels;
  stbi_uc *pixels=stbi_load_from_memory(data,(int)size,&width,&height,&channels,STBI_rgb_alpha);
  if(!pixels){
    const char *error=stbi_failure_reason();
    std::cerr<<"[3DEngine] Failed to decode embedded texture: "
      <<(error?error:"unknown error")<<std::endl;
    return 0;
  }
  unsigned int texture=uploadTexture(pixels,width,height);
  stbi_image_free(pixels);
  return texture;
}
