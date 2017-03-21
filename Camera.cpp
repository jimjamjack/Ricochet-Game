// Include GLFW
#include <GLFW/glfw3.h>
extern GLFWwindow* window; // The "extern" keyword here is to access the variable "window" declared in Main.cpp

// Include GLM
#include <glm/glm.hpp>
#include "glm/ext.hpp"
using namespace glm;

#include <iostream>
using namespace std;

#include "Camera.h"

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;
glm::vec3 direction;

glm::mat4 getViewMatrix() {
	return ViewMatrix;
}
glm::mat4 getProjectionMatrix() {
	return ProjectionMatrix;
}

// Initial position : on +Z
glm::vec3 position = glm::vec3(1, 0, 1);
// Initial horizontal angle : toward -Z
float horizontalAngle = 3.14f;
// Initial vertical angle : none
float verticalAngle = 0.0f;
// Initial Field of View
float initialFoV = 45.0f;

float speed = 5.0f; // units / second
float mouseSpeed = 0.005f;

glm::vec3 getPosition() {
	return position;
}

glm::vec3 getDirection() {
	return direction;
}

float getHorizontal() {
	return horizontalAngle;
}

float getVertical() {
	return verticalAngle;
}

void computeMatricesFromInputs() {

	// glfwGetTime is called only once, the first time this function is called
	static double lastTime = glfwGetTime();

	// Compute time difference between current and last frame
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);

	// Get mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Reset mouse position for next frame
	int windowWidth, windowHeight;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);
	glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);

	// Compute new orientation
	horizontalAngle += mouseSpeed * float(windowWidth / 2 - xpos);
	verticalAngle += mouseSpeed * float(windowHeight / 2 - ypos);
	if (verticalAngle > 1.5708) verticalAngle = 1.5708;
	if (verticalAngle < -1.5708) verticalAngle = -1.5708;

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	glm::vec3 direction(
		cos(verticalAngle) * sin(horizontalAngle),
		sin(verticalAngle),
		cos(verticalAngle) * cos(horizontalAngle)
	);

	// Right vector
	glm::vec3 right = glm::vec3(
		sin(horizontalAngle - 3.14f / 2.0f),
		0,
		cos(horizontalAngle - 3.14f / 2.0f)
	);

	// Up vector
	glm::vec3 up = glm::cross(right, direction);

	glm::vec3 nextStep = position;

		// Move forward
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			nextStep += direction * deltaTime * speed;
			nextStep.y = 0; // Forces user to remain on ground
		}
		// Move backward
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			nextStep -= direction * deltaTime * speed;
			nextStep.y = 0;
		}
		// Strafe right
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			nextStep += right * deltaTime * speed;
		}
		// Strafe left
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			nextStep -= right * deltaTime * speed;
		}

		if (nextStep.z < 9 && nextStep.z > -7 && nextStep.x < 9 && nextStep.x > -7) {
			position = nextStep;
		}

	float FoV = initialFoV;

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit - 100 units
	ProjectionMatrix = glm::perspective(FoV, 4.0f / 3.0f, 0.1f, 100.0f);
	// Camera matrix
	ViewMatrix = glm::lookAt(
		position,             // Camera is here
		position + direction, // and looks here : at the same position, plus "direction"
		up                    // Head is up
	);

	// For the next frame, the "last time" will be "now"
	lastTime = currentTime;
};