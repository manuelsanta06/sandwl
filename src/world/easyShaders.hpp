#pragma once

#include <GLES2/gl2.h>
  
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
  void setFloat(const std::string &name,float value)const;
  void setBool(const std::string &name,bool value)const;
  void setInt(const std::string &name,int value)const;
};

