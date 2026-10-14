#include "easyShaders.hpp"

#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


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


EasyProgram::EasyProgram(const char* vertexPath,const char* fragmentPath){load(vertexPath,fragmentPath);}
void EasyProgram::load(const char* vertexPath,const char* fragmentPath){
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

EasyProgram::~EasyProgram(){
  glDeleteProgram(ID);
}


void EasyProgram::use(){
  glUseProgram(ID);
}

void EasyProgram::setUniform(const std::string &name,bool value)const{
  glUniform1i(glGetUniformLocation(ID,name.c_str()),(int)value);
}
void EasyProgram::setUniform(const std::string &name,int value)const{
  glUniform1i(glGetUniformLocation(ID,name.c_str()),value);
}
void EasyProgram::setUniform(const std::string &name,float value)const{
  glUniform1f(glGetUniformLocation(ID,name.c_str()),value);
}

void EasyProgram::setUniform(const std::string &name,const glm::vec2 &value)const{
  glUniform2fv(glGetUniformLocation(ID,name.c_str()),1,&value[0]); 
}
void EasyProgram::setUniform(const std::string &name,float x,float y)const{
  glUniform2f(glGetUniformLocation(ID,name.c_str()),x,y); 
}
void EasyProgram::setUniform(const std::string &name,const glm::vec3 &value)const{
  glUniform3fv(glGetUniformLocation(ID,name.c_str()),1,&value[0]); 
}
void EasyProgram::setUniform(const std::string &name,float x,float y,float z)const{
  glUniform3f(glGetUniformLocation(ID,name.c_str()),x,y,z); 
}
void EasyProgram::setUniform(const std::string &name,const glm::vec4 &value)const{
  glUniform4fv(glGetUniformLocation(ID,name.c_str()),1,&value[0]); 
}
void EasyProgram::setUniform(const std::string &name,float x,float y,float z,float w)const{
  glUniform4f(glGetUniformLocation(ID,name.c_str()),x,y,z,w); 
}

void EasyProgram::setUniform(const std::string &name,const glm::mat2 &mat)const{
  glUniformMatrix2fv(glGetUniformLocation(ID,name.c_str()),1,GL_FALSE,glm::value_ptr(mat));
}
void EasyProgram::setUniform(const std::string &name,const glm::mat3 &mat)const{
  glUniformMatrix3fv(glGetUniformLocation(ID,name.c_str()),1,GL_FALSE,glm::value_ptr(mat));
}
void EasyProgram::setUniform(const std::string &name,const glm::mat4 &mat)const{
  glUniformMatrix4fv(glGetUniformLocation(ID,name.c_str()),1,GL_FALSE,glm::value_ptr(mat));
}
