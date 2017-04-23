// Include GLFW
#include <GLFW/glfw3.h>
extern GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include "glm/ext.hpp"
using namespace glm;

#include <iostream>
using namespace std;

#include "Camera.h"

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

// Initial starting position
glm::vec3 position = glm::vec3(1, 0, 1);
// Initial starting horizontal angle (facing the -Z axis)
float horizontalAngle = 3.14f;
// Initial starting vertical angle (Facing straight ahead)
float verticalAngle = 0.0f;
// Field of View
float fieldOfView = 45.0f;
// 4:3 aspect ratio
float aspectRatio = 4.0f / 3.0f;
// Minimum clip distance
float nearClip = 0.1f;
// Maximum clip distance
float farClip = 100.0f;

// Speed is in units per seconds
float speed = 5.0f;
float mouseSpeed = 0.005f;

glm::mat4 getViewMatrix() {
	return ViewMatrix;
}
glm::mat4 getProjectionMatrix() {
	return ProjectionMatrix;
}

float getHorizontal() {
	return horizontalAngle;
}

float getVertical() {
	return verticalAngle;
}

void updateCamera() {

	// Get the time at the start of the function call
	static double previousTime = glfwGetTime();

	// Get the difference between the current time and previous time
	double currentTime = glfwGetTime();
	float deltaTime = (float)currentTime - (float)previousTime;

	// Get mouse position
	double xMousePosition, yMousePosition;
	glfwGetCursorPos(window, &xMousePosition, &yMousePosition);

	// Get window size
	int windowWidth, windowHeight;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	// Reset mouse position for next frame
	glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);

	// Compute new orientation using distance of cursor from centre screen
	horizontalAngle += mouseSpeed * float(windowWidth / 2 - xMousePosition);
	verticalAngle += mouseSpeed * float(windowHeight / 2 - yMousePosition);
	// Restrict vertical angle so that the player can only look 180 degrees up and down
	if (verticalAngle > 1.5708) verticalAngle = 1.5708f;
	if (verticalAngle < -1.5708) verticalAngle = -1.5708f;

	// Convert the spherical co-ordinates to cartesian co-ordinates
	glm::vec3 direction(cos(verticalAngle) * sin(horizontalAngle), sin(verticalAngle), cos(verticalAngle) * cos(horizontalAngle));

	// Right vector (horizontal angle - 90°)
	glm::vec3 right = glm::vec3(sin(horizontalAngle - 3.14f/2.0f), 0, cos(horizontalAngle - 3.14f/2.0f));

	// Up vector (result of the cross-product of right and direction vectors)
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

	// Check that the user's next step is within the room
	if (nextStep.z < 9 && nextStep.z > -7 && nextStep.x < 9 && nextStep.x > -7) {
		position = nextStep;
	}

	// Set projection matrix
	ProjectionMatrix = glm::perspective(fieldOfView, aspectRatio, nearClip, farClip);
	// Set updated camera matrix
	ViewMatrix = glm::lookAt(position, position + direction, up);

	previousTime = currentTime;
};