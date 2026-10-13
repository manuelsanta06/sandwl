#include "easyShaders.hpp"

#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>


std::string readShaderFile(const std::string& filePath){
  std::ifstream file(filePath);
  if(!file.is_open()){
    std::cerr<<"[3DEngine] Error: no such shader file"<<filePath<<std::endl;
    return "";
  }

  std::stringstream buffer;
  buffer<<file.rdbuf();
  file.close();

  return buffer.str();
}

bool loadShader(unsigned int &id,int type,const std::string& filePath){
  id=glCreateShader(type);
  std::string rawCode=readShaderFile(filePath);
  const char* castedCode=rawCode.c_str();

  glShaderSource(id,1,&castedCode,NULL);
  glCompileShader(id);

  int  success;
  char infoLog[512];
  glGetShaderiv(id,GL_COMPILE_STATUS,&success);
  if(!success){
    glGetShaderInfoLog(id,512,NULL,infoLog);
    std::cerr<<"[3Dengine] ERROR::SHADER::COMPILATION_FAILED file"<<filePath<<"\n"<<infoLog<<std::endl;
    return false;
  }
  return true;
}


EasyProgram::EasyProgram(const char* vertexPath,const char* fragmentPath){
  unsigned int Sfragment;
  unsigned int Svertex;
  loadShader(Svertex,GL_VERTEX_SHADER,vertexPath);
  loadShader(Sfragment,GL_FRAGMENT_SHADER,fragmentPath);

  ID=glCreateProgram();
  glAttachShader(ID,Svertex);
  glAttachShader(ID,Sfragment);
  glLinkProgram(ID);

  glDeleteShader(Svertex);
  glDeleteShader(Sfragment);
}

void EasyProgram::use(){
  glUseProgram(ID);
}

void EasyProgram::setBool(const std::string &name,bool value)const{
  glUniform1i(glGetUniformLocation(ID,name.c_str()),(int)value);
}
void EasyProgram::setInt(const std::string &name,int value)const{
  glUniform1i(glGetUniformLocation(ID,name.c_str()),value);
}
void EasyProgram::setFloat(const std::string &name,float value)const{
  glUniform1f(glGetUniformLocation(ID,name.c_str()),value);
}
