#pragma once

#include "world/engineApi.h"

#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//temporal hardcoded defaults
const float YAW         =-90.0f;
const float PITCH       = 0.0f;
const float SPEED       = 2.5f;
const float ZOOM        = 45.0f;

class Camera{
public:
  glm::vec3 m_position;
private:
  glm::vec3 m_front;
  glm::vec3 m_up;
  glm::vec3 m_right;
  glm::vec3 m_worldUp;
  //euler angles
  float m_yaw;
  float m_pitch;
  //TODO: roll?
  //camera options
  float m_movementSpeed;
  float m_zoom;

public:
  Camera(glm::vec3 position,glm::vec3 up=glm::vec3(0.0f,1.0f,0.0f),float yaw=YAW,float pitch=PITCH)
    :m_front(glm::vec3(0.0f,0.0f,-1.0f)),m_movementSpeed(SPEED),m_zoom(ZOOM)
  {
    m_position=position;
    m_worldUp=up;
    m_yaw=yaw;
    m_pitch=pitch;
    updateCameraVectors();
  }
  // constructor with scalar values
  Camera(float posX,float posY,float posZ,float upX,float upY,float upZ,float yaw,float pitch)
    :m_front(glm::vec3(0.0f,0.0f,-1.0f)),m_movementSpeed(SPEED),m_zoom(ZOOM)
  {
    m_position=glm::vec3(posX,posY,posZ);
    m_worldUp=glm::vec3(upX,upY,upZ);
    m_yaw=yaw;
    m_pitch=pitch;
    updateCameraVectors();
  }

  glm::mat4 GetViewMatrix()const{
    return glm::lookAt(m_position,m_position+m_front,m_up);
  }

  void processInput(const EngineInput *input,float deltaTime){
    glm::vec3 walkMovement;
    walkMovement=m_front*input->forward+m_right*input->right;
    float length=glm::length(walkMovement);
    if(length>1)walkMovement/=length;
    walkMovement*=m_movementSpeed*deltaTime;
    m_position+=walkMovement;

    m_yaw+=input->yaw_delta;
    m_pitch+=input->pitch_delta;
    m_pitch=glm::clamp(m_pitch,-89.0f,89.0f);
    updateCameraVectors();
  }

private:
  void updateCameraVectors(){
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front,m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
  }
};
