#ifndef CAMERA_H
#define CAMERA_H

float getHorizontal();
float getVertical();
void computeMatricesFromInputs();
glm::vec3 getPosition();
glm::vec3 getDirection();
glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();

#endif