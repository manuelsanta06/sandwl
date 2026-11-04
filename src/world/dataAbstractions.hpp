#pragma once

#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <vector>

#include "easyShaders.hpp"

struct Vertex{
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
};

enum class TextureType{
  DIFFUSE,
  SPECULAR
};

struct Texture{
  unsigned int id;
  TextureType type;
  std::string path;
};

class Mesh{
public:
  std::vector<Vertex>       vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture>      textures;

  Mesh(std::vector<Vertex> vertices,std::vector<unsigned int> indices,std::vector<Texture> textures)
  :vertices(vertices),indices(indices),textures(textures){
    setupMesh();
  }
  void draw(EasyProgram &program)const;
private:
  unsigned int VAO,VBO,EBO;

  void setupMesh();
};

class Model{
public:
  Model(const char *path,bool flipUVs=true){
    if(path!=0)loadModel(path,flipUVs);
  }
  void draw(EasyProgram &program)const;
private:
  std::vector<Mesh> meshes;
  std::vector<Texture> textures_loaded;
  std::string directory;

  void loadModel(std::string path,bool flipUVs);
  void processNode(aiNode *node,const aiScene *scene);
  Mesh processMesh(aiMesh *mesh,const aiScene *scene);
  std::vector<Texture> loadMaterialTextures(aiMaterial *mat,const aiScene *scene,aiTextureType aiType,TextureType type);
};
