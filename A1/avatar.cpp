#include "avatar.hpp"

Avatar::Avatar(){
  posX = -1;
  posY = -1;
  color = glm::vec3(1, 0, 0);
}

Avatar::~Avatar(){}

float Avatar::getPositionX(){
  return posX;
}

float Avatar::getPositionY(){
  return posY;
}

void Avatar::setPositionX(float x){
  posX = x;
}

void Avatar::setPositionY(float y){
  posY = y;
}

glm::vec3 Avatar::getColor(){
  return color;
}

void Avatar::setColor(glm::vec3 colorVec){
  color = colorVec;
}
