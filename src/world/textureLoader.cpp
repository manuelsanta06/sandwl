#include "textureLoader.hpp"

#include <GLES3/gl3.h>
#include <png.h>

#include <algorithm>
#include <iostream>
#include <vector>

unsigned int loadTexture(const char *path){
  png_image image{};
  image.version=PNG_IMAGE_VERSION;
  if(!png_image_begin_read_from_file(&image,path)){
    std::cerr<<"[3DEngine] Failed to read texture "<<path<<": "<<image.message<<std::endl;
    return 0;
  }

  image.format=PNG_FORMAT_RGBA;
  std::vector<png_byte> pixels(PNG_IMAGE_SIZE(image));
  if(!png_image_finish_read(&image,nullptr,pixels.data(),0,nullptr)){
    std::cerr<<"[3DEngine] Failed to decode texture "<<path<<": "<<image.message<<std::endl;
    png_image_free(&image);
    return 0;
  }

  size_t rowStride=(size_t)image.width*4;
  for(png_uint_32 y=0;y<image.height/2;y++){
    auto top=pixels.begin()+y*rowStride;
    auto bottom=pixels.begin()+(image.height-1-y)*rowStride;
    std::swap_ranges(top,top+rowStride,bottom);
  }

  glActiveTexture(GL_TEXTURE0);

  GLuint texture=0;
  glGenTextures(1,&texture);
  glBindTexture(GL_TEXTURE_2D,texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,image.width,image.height,0,GL_RGBA,
    GL_UNSIGNED_BYTE,pixels.data());
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  png_image_free(&image);
  return texture;
}
