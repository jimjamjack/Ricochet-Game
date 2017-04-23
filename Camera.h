#ifndef CAMERA_H
#define CAMERA_H

glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();

float getHorizontal();
float getVertical();

void updateCamera();

#endif