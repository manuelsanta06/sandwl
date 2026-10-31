#include <string>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#import "dataAbstractions.hpp"
#include "textureLoader.hpp"
#include "easyShaders.hpp"

void Mesh::draw(EasyProgram &program)const{
  unsigned int diffuseNr=1;
  unsigned int specularNr=1;
  for(unsigned int i=0;i<textures.size(); i++){
    glActiveTexture(GL_TEXTURE0+i);
    std::string number;
    std::string name;
    switch(textures[i].type){
      case TextureType::DIFFUSE:
        number=std::to_string(diffuseNr++);
        name="texture_diffuse";
        break;
      case TextureType::SPECULAR:
        number=std::to_string(specularNr++);
        name="texture_speculr";
        break;
      default:
        number="0";
        name="texture_specular";
        break;
    }

    program.setUniform(("material."+name+number).c_str(),(int)i);
    glBindTexture(GL_TEXTURE_2D,textures[i].id);
  }
  glActiveTexture(GL_TEXTURE0);

  // draw mesh
  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES,indices.size(),GL_UNSIGNED_INT,0);
  glBindVertexArray(0);
}

void Mesh::setupMesh(){
  glGenVertexArrays(1,&VAO);
  glGenBuffers(1,&VBO);
  glGenBuffers(1,&EBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER,VBO);

  glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(unsigned int),&indices[0],GL_STATIC_DRAW);

  //vertex positions
  glEnableVertexAttribArray(0);	
  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);
  //vertex normals
  glEnableVertexAttribArray(1);	
  glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,Normal));
  //vertex texture coords
  glEnableVertexAttribArray(2);	
  glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,TexCoords));

  glBindVertexArray(0);
}


void Model::draw(EasyProgram &program)const{
  for(const auto& i:meshes){
    i.draw(program);
  }
}

void Model::loadModel(std::string path){
  Assimp::Importer import;
  const aiScene *scene=import.ReadFile(path,aiProcess_Triangulate|aiProcess_FlipUVs);
  if(!scene||scene->mFlags&AI_SCENE_FLAGS_INCOMPLETE||!scene->mRootNode){
    std::cerr<<"ERROR::ASSIMP::"<<import.GetErrorString()<<std::endl;
    return;
  }
  directory=path.substr(0,path.find_last_of('/'));
}

void Model::processNode(aiNode *node,const aiScene *scene){
  //process all the nodes meshes(if any)
  for(unsigned int i=0;i<node->mNumMeshes;i++){
    aiMesh *mesh=scene->mMeshes[node->mMeshes[i]];
    meshes.push_back(processMesh(mesh,scene));			
  }
  for(unsigned int i=0;i<node->mNumChildren;i++){
    processNode(node->mChildren[i],scene);
  }
}

Mesh Model::processMesh(aiMesh *mesh,const aiScene *scene){
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;

  for(unsigned int i=0;i<mesh->mNumVertices;i++){
    Vertex vertex;
    glm::vec3 vector; 
    vector.x=mesh->mVertices[i].x;
    vector.y=mesh->mVertices[i].y;
    vector.z=mesh->mVertices[i].z; 
    vertex.Position=vector;

    vector.x=mesh->mNormals[i].x;
    vector.y=mesh->mNormals[i].y;
    vector.z=mesh->mNormals[i].z;
    vertex.Normal=vector;

    if(mesh->mTextureCoords[0]){
      glm::vec2 vec;
      vec.x=mesh->mTextureCoords[0][i].x;
      vec.y=mesh->mTextureCoords[0][i].y;
      vertex.TexCoords=vec;
    }else
      vertex.TexCoords=glm::vec2(0.0f,0.0f);

    vertices.push_back(vertex);
  }
  // process indices
  for(unsigned int i=0;i<mesh->mNumFaces;i++){
    aiFace face=mesh->mFaces[i];
    for(unsigned int j=0;j<face.mNumIndices;j++)
      indices.push_back(face.mIndices[j]);
  }
  // process material
  if(mesh->mMaterialIndex>=0){
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    std::vector<Texture> diffuseMaps=loadMaterialTextures(material,aiTextureType_DIFFUSE,"texture_diffuse");
    textures.insert(textures.end(),diffuseMaps.begin(),diffuseMaps.end());
    std::vector<Texture> specularMaps=loadMaterialTextures(material,aiTextureType_SPECULAR,"texture_specular");
    textures.insert(textures.end(),specularMaps.begin(),specularMaps.end());
  }

  return Mesh(vertices,indices,textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat,aiTextureType type,std::string typeName){
  std::vector<Texture> textures;
  for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
  {
      aiString str;
      mat->GetTexture(type, i, &str);
      Texture texture;
      // texture.id = TextureFromFile(str.C_Str(), directory);
      // texture.type = typeName;
      // texture.path = str;
      textures.push_back(texture);
  }
  return textures;
}

