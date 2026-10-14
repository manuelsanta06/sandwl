#pragma once

#include <GLES2/gl2.h>
#include <glm/glm.hpp>
  
#include <string>

std::string readShaderFile(const std::string& filePath);
bool loadShader(unsigned int &id,int type,const std::string& filePath);


class EasyProgram{
public:
  unsigned int ID;

  EasyProgram()=default;
  ~EasyProgram();
  EasyProgram(const char* vertexPath,const char* fragmentPath);

  void load(const char* vertexPath,const char* fragmentPath);

  void use();

  //easy uniforms
  void setUniform(const std::string &name,float value)const;
  void setUniform(const std::string &name,bool value)const;
  void setUniform(const std::string &name,int value)const;

  void setUniform(const std::string &name,const glm::vec2 &value)const;
  void setUniform(const std::string &name,float x,float y)const;
  
  void setUniform(const std::string &name,const glm::vec3 &value)const;
  void setUniform(const std::string &name,float x,float y,float z)const;
  
  void setUniform(const std::string &name,const glm::vec4 &value)const;
  void setUniform(const std::string &name,float x,float y,float z,float w)const;

  void setUniform(const std::string &name,const glm::mat2 &mat)const;
  void setUniform(const std::string &name,const glm::mat3 &mat)const;
  void setUniform(const std::string &name,const glm::mat4 &mat)const;
};

