#include <filesystem>
#include <iostream>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "dataAbstractions.hpp"
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
        name="texture_specular";
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

void Model::loadModel(std::string path,bool flipUVs){
  Assimp::Importer import;
  unsigned int flags=aiProcess_Triangulate;
  if(flipUVs)flags|=aiProcess_FlipUVs;
  const aiScene *scene=import.ReadFile(path,flags);
  if(!scene||scene->mFlags&AI_SCENE_FLAGS_INCOMPLETE||!scene->mRootNode){
    std::cerr<<"ERROR::ASSIMP::"<<import.GetErrorString()<<std::endl;
    return;
  }
  directory=path.substr(0,path.find_last_of('/'));
  processNode(scene->mRootNode,scene);
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
  for(unsigned int i=0;i<mesh->mNumFaces;i++){
    aiFace face=mesh->mFaces[i];
    for(unsigned int j=0;j<face.mNumIndices;j++)
      indices.push_back(face.mIndices[j]);
  }
  if(mesh->mMaterialIndex<scene->mNumMaterials){
    aiMaterial *material=scene->mMaterials[mesh->mMaterialIndex];
    std::vector<Texture> diffuseMaps=loadMaterialTextures(material,scene,aiTextureType_DIFFUSE,TextureType::DIFFUSE);
    textures.insert(textures.end(),diffuseMaps.begin(),diffuseMaps.end());
    std::vector<Texture> specularMaps=loadMaterialTextures(material,scene,aiTextureType_SPECULAR,TextureType::SPECULAR);
    textures.insert(textures.end(),specularMaps.begin(),specularMaps.end());
  }

  return Mesh(vertices,indices,textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat,const aiScene *scene,aiTextureType aiType,TextureType type){
  std::vector<Texture> textures;
  for(unsigned int i=0;i<mat->GetTextureCount(aiType);i++){
    aiString str;
    if(mat->GetTexture(aiType,i,&str)!=AI_SUCCESS)continue;
    bool skip=false;
    for(unsigned int j=0;j<textures_loaded.size();j++){
      if(std::strcmp(textures_loaded[j].path.data(),str.C_Str())==0){
        textures.push_back(textures_loaded[j]);
        skip=true;
        break;
      }
    }
    if(!skip){
      Texture texture;
      const aiTexture *embedded=scene->GetEmbeddedTexture(str.C_Str());
      if(embedded){
        if(embedded->mHeight!=0){
          std::cerr<<"[3DEngine] Uncompressed embedded texture is unsupported: "<<str.C_Str()<<std::endl;
          continue;
        }
        texture.id=loadTextureMemory(reinterpret_cast<const unsigned char*>(embedded->pcData),embedded->mWidth);
      }else{
        std::filesystem::path texturePath=std::filesystem::path(directory)/str.C_Str();
        texture.id=loadTextureFile(texturePath.c_str());
      }
      if(!texture.id)continue;
      texture.type=type;
      texture.path=str.C_Str();
      textures.push_back(texture);
      textures_loaded.push_back(texture);
    }
  }
  return textures;
}

