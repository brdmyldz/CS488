#pragma once

#include <glm/glm.hpp>

class Avatar
{
public:
  Avatar();
	~Avatar();

  float getPositionX();
  float getPositionY();
  void setPositionX(float x);
  void setPositionY(float y);
  glm::vec3 getColor();
  void setColor(glm::vec3 colorVec);

private:
  float posX;
  float posY;
  glm::vec3 color;
};
