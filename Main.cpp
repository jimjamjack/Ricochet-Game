// Include standard headers
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
using namespace std;

// Include GLEW
#include <GL/glew.h>

// include GLUT
#include <GL/glut.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext.hpp"
using namespace glm;

// Include Bullet
#include <btBulletDynamicsCommon.h>
#include <LinearMath/btIDebugDraw.h>

// Include my files
#include "Shader.h"
#include "Camera.h"
#include "ObjLoader.h"
#include "Texture.h"
#include "DebugDrawer.h"

bool debugmode = false;
int targetNum = 5;
int bulletCount = 2;
int shots = 0;
int gravity = -9.81f;

void initialiseGame() {
	string useDefault;
	string useDebug;
	int enteredNumber = 0;
	string enteredGravity;

	while (useDefault != "y" && useDefault != "n") {
		cout << "Would you like to use default settings? (y/n): ";
		getline(cin, useDefault);
		if (useDefault == "n") {

			while (useDebug != "y" && useDebug != "n") {
				cout << "Would you like to use debug mode? (y/n): ";
				getline(cin, useDebug);
				if (useDebug == "y") {
					debugmode = true;
				}
			}

			while (!(enteredNumber >= 3 && enteredNumber <= 15)) {
				cout << "How many targets would you like? (3 - 15): ";
				cin >> enteredNumber;
				while (cin.fail())
				{
					cout << "That was not an integer! Please enter an integer (3 - 15): ";
					cin.clear();
					cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					cin >> enteredNumber;
				}
				targetNum = enteredNumber;
				bulletCount = floor(targetNum / 2);
				cin.ignore();
			}

			while (enteredGravity != "e" && enteredGravity != "m" && enteredGravity != "j") {
				cout << "Which gravity would you like? ((e)arth, (m)oon or (j)upiter): ";
				getline(cin, enteredGravity);
				if (enteredGravity == "e") {
					gravity = -9.81f;
				}
				else if (enteredGravity == "m") {
					gravity = -1.6f;
				}
				else if (enteredGravity == "j") {
					gravity = -24.8;
				}
			}
		}
	}
}

void drawCrosshair() {
	int windowWidth, windowHeight;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

}

void generateTargets(std::vector<glm::vec3> &positions, std::vector<glm::quat> &orientations, int targetNum) {

	for (int i = 0; i < targetNum; i++) {
		int x = rand() % 12 - 5;
		int y = rand() % 5;
		int z = rand() % 12 - 5;
		for (auto &position : positions) {
			while ((position.x <= x + 3) && (position.x >= x - 3) &&
				(position.y <= y + 3) && (position.y >= y - 3) &&
				(position.z <= z + 3) && (position.z >= z - 3)) {

				x = rand() % 12 - 5;
				y = rand() % 5;
				z = rand() % 12 - 5;
			}
		}
		positions[i] = glm::vec3(x, y, z);
		orientations[i] = glm::normalize(glm::quat(glm::vec3(rand() % 60, rand() % 60, rand() % 60)));
	}

}

void generateWalls(std::vector<glm::vec3> &positions, std::vector<glm::quat> &orientations) {

	positions[0] = glm::vec3(1.02, 2.1, 9.5);
	orientations[0] = glm::normalize(glm::quat(glm::vec3(0, 0, 0)));

	positions[1] = glm::vec3(1.02, 2.1, -7.45);
	orientations[1] = glm::normalize(glm::quat(glm::vec3(0, 0, 0)));

	positions[2] = glm::vec3(-7.5, 2.1, 1.02);
	orientations[2] = glm::normalize(glm::quat(glm::vec3(0, 1.5708, 0)));

	positions[3] = glm::vec3(9.54, 2.1, 1.02);
	orientations[3] = glm::normalize(glm::quat(glm::vec3(0, 1.5708, 0)));

	positions[4] = glm::vec3(1.02, 6.9, 1.02);
	orientations[4] = glm::normalize(glm::quat(glm::vec3(0, 0, 0)));

	positions[5] = glm::vec3(1.02, -2.7, 1.02);
	orientations[5] = glm::normalize(glm::quat(glm::vec3(0, 0, 0)));
}

void ScreenPosToWorldRay(
	int mouseX, int mouseY,             // Mouse position, in pixels, from bottom-left corner of the window
	int screenWidth, int screenHeight,  // Window size, in pixels
	glm::mat4 ViewMatrix,               // Camera position and orientation
	glm::mat4 ProjectionMatrix,         // Camera parameters (ratio, field of view, near and far planes)
	glm::vec3& out_origin,              // Ouput : Origin of the ray.
	glm::vec3& out_direction            // Ouput : Direction, in world space, of the ray that goes "through" the mouse.
) {

	// The ray Start and End positions, in Normalized Device Coordinates
	glm::vec4 lRayStart_NDC(
		((float)mouseX / (float)screenWidth - 0.5f) * 2.0f, // [0,1024] -> [-1,1]
		((float)mouseY / (float)screenHeight - 0.5f) * 2.0f, // [0, 768] -> [-1,1]
		-1.0, // The near plane maps to Z=-1 in Normalized Device Coordinates
		1.0f
	);
	glm::vec4 lRayEnd_NDC(
		((float)mouseX / (float)screenWidth - 0.5f) * 2.0f,
		((float)mouseY / (float)screenHeight - 0.5f) * 2.0f,
		0.0,
		1.0f
	);

	// The Projection matrix goes from Camera Space to NDC.
	// So inverse(ProjectionMatrix) goes from NDC to Camera Space.
	glm::mat4 InverseProjectionMatrix = glm::inverse(ProjectionMatrix);

	// The View Matrix goes from World Space to Camera Space.
	// So inverse(ViewMatrix) goes from Camera Space to World Space.
	glm::mat4 InverseViewMatrix = glm::inverse(ViewMatrix);

	glm::vec4 lRayStart_camera = InverseProjectionMatrix * lRayStart_NDC;    lRayStart_camera /= lRayStart_camera.w;
	glm::vec4 lRayStart_world = InverseViewMatrix       * lRayStart_camera; lRayStart_world /= lRayStart_world.w;
	glm::vec4 lRayEnd_camera = InverseProjectionMatrix * lRayEnd_NDC;      lRayEnd_camera /= lRayEnd_camera.w;
	glm::vec4 lRayEnd_world = InverseViewMatrix       * lRayEnd_camera;   lRayEnd_world /= lRayEnd_world.w;

	glm::vec3 lRayDir_world(lRayEnd_world - lRayStart_world);
	lRayDir_world = glm::normalize(lRayDir_world);

	out_origin = glm::vec3(lRayStart_world);
	out_direction = glm::normalize(lRayDir_world);
}

//void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
//{
//	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
//		printf("\nRight Click");
//	}
//}

int main()
{

	initialiseGame();

	// Initialise GLFW
	if (!glfwInit())
	{
		printf("Failed to initialize GLFW.\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, false);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Ricochet Game", NULL, NULL);
	if (window == NULL) {
		printf("Failed to open GLFW window.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		printf("Failed to initialize GLEW.\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	int windowWidth, windowHeight;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	// Ensure we can capture the escape key being pressed (key remains in GLFW_PRESS state until glfwGetKey)
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Create a crosshair and use it as the default cursor
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	GLFWcursor* crosshairCursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
	glfwSetCursor(window, crosshairCursor);

	// Tell GLFW what to do on left click
	//glfwSetMouseButtonCallback(window, mouse_button_callback);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);

	// Basic grey background
	glClearColor(0.827f, 0.827f, 0.827f, 0.0f);

	// Enable depth test so drawn triangles don't overlap
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it's closer to the camera than the former one
	glDepthFunc(GL_LESS);

	GLuint VertexArrayID;
	glGenVertexArrays(
		1,				// Number of vertex array object names to generate
		&VertexArrayID  // Specifies an array in which the generated vertex array object names are stored
	);
	glBindVertexArray(VertexArrayID);

	// Create and compile the GLSL program from the shaders
	GLuint programID = LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader");

	// Load the textures
	GLuint roomTexture = loadBMP_custom("roomTexture.bmp");
	GLuint bulletTexture = loadBMP_custom("bulletTexture.bmp");
	GLuint targetTexture = loadBMP_custom("targetTexture.bmp");
	GLuint gunTexture = loadBMP_custom("gunTexture.bmp");

	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Read our .obj files
	std::vector<unsigned short> roomIndices;
	std::vector<glm::vec3> roomVertices;
	std::vector<glm::vec2> roomUvs;
	std::vector<glm::vec3> roomNormals;
	bool res = loadAssImp("room.obj", roomIndices, roomVertices, roomUvs, roomNormals);

	std::vector<unsigned short> bulletIndices;
	std::vector<glm::vec3> bulletVertices;
	std::vector<glm::vec2> bulletUvs;
	std::vector<glm::vec3> bulletNormals;
	res = loadAssImp("bullet2.obj", bulletIndices, bulletVertices, bulletUvs, bulletNormals);

	std::vector<unsigned short> targetIndices;
	std::vector<glm::vec3> targetVertices;
	std::vector<glm::vec2> targetUvs;
	std::vector<glm::vec3> targetNormals;
	res = loadAssImp("target2.obj", targetIndices, targetVertices, targetUvs, targetNormals);

	std::vector<unsigned short> gunIndices;
	std::vector<glm::vec3> gunVertices;
	std::vector<glm::vec2> gunUvs;
	std::vector<glm::vec3> gunNormals;
	res = loadAssImp("gun2.obj", gunIndices, gunVertices, gunUvs, gunNormals);

	// Load it into a VBO
	// First object

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, roomVertices.size() * sizeof(glm::vec3), &roomVertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, roomUvs.size() * sizeof(glm::vec2), &roomUvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, roomNormals.size() * sizeof(glm::vec3), &roomNormals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, roomIndices.size() * sizeof(unsigned short), &roomIndices[0], GL_STATIC_DRAW);

	// Second object
	GLuint vertexbuffer2;
	glGenBuffers(1, &vertexbuffer2);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer2);
	glBufferData(GL_ARRAY_BUFFER, bulletVertices.size() * sizeof(glm::vec3), &bulletVertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer2;
	glGenBuffers(1, &uvbuffer2);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer2);
	glBufferData(GL_ARRAY_BUFFER, bulletUvs.size() * sizeof(glm::vec2), &bulletUvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer2;
	glGenBuffers(1, &normalbuffer2);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer2);
	glBufferData(GL_ARRAY_BUFFER, bulletNormals.size() * sizeof(glm::vec3), &bulletNormals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer2;
	glGenBuffers(1, &elementbuffer2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer2);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, bulletIndices.size() * sizeof(unsigned short), &bulletIndices[0], GL_STATIC_DRAW);

	// Third object

	GLuint vertexbuffer3;
	glGenBuffers(1, &vertexbuffer3);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer3);
	glBufferData(GL_ARRAY_BUFFER, targetVertices.size() * sizeof(glm::vec3), &targetVertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer3;
	glGenBuffers(1, &uvbuffer3);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer3);
	glBufferData(GL_ARRAY_BUFFER, targetUvs.size() * sizeof(glm::vec2), &targetUvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer3;
	glGenBuffers(1, &normalbuffer3);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer3);
	glBufferData(GL_ARRAY_BUFFER, targetNormals.size() * sizeof(glm::vec3), &targetNormals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer3;
	glGenBuffers(1, &elementbuffer3);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer3);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, targetIndices.size() * sizeof(unsigned short), &targetIndices[0], GL_STATIC_DRAW);

	// fourth object

	GLuint vertexbuffer4;
	glGenBuffers(1, &vertexbuffer4);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer4);
	glBufferData(GL_ARRAY_BUFFER, gunVertices.size() * sizeof(glm::vec3), &gunVertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer4;
	glGenBuffers(1, &uvbuffer4);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer4);
	glBufferData(GL_ARRAY_BUFFER, gunUvs.size() * sizeof(glm::vec2), &gunUvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer4;
	glGenBuffers(1, &normalbuffer4);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer4);
	glBufferData(GL_ARRAY_BUFFER, gunNormals.size() * sizeof(glm::vec3), &gunNormals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer4;
	glGenBuffers(1, &elementbuffer4);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer4);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, gunIndices.size() * sizeof(unsigned short), &gunIndices[0], GL_STATIC_DRAW);

	// Seed the rand() function for more random results
	srand(time(NULL));

	// Generate positions & rotations for n targets
	std::vector<glm::vec3> targetPositions(targetNum);
	std::vector<glm::quat> targetOrientations(targetNum);
	generateTargets(targetPositions, targetOrientations, targetNum);

	// Generate room surface positions & rotations
	std::vector<glm::vec3> wallPositions(6);
	std::vector<glm::quat> wallOrientations(6);
	generateWalls(wallPositions, wallOrientations);

	// Generate bullet positions & rotations
	std::vector<glm::vec3> bulletPositions(bulletCount);
	std::vector<glm::quat> bulletRotations(bulletCount);
	std::vector<btRigidBody*> bulletRigidBodies(bulletCount);

	// Initialize Bullet

	// Build the broadphase
	btBroadphaseInterface* broadphase = new btDbvtBroadphase();

	// Set up the collision configuration and dispatcher
	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);

	// The actual physics solver
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

	// The world
	btDiscreteDynamicsWorld* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, gravity, 0));

	BulletDebugDrawer_OpenGL mydebugdrawer;
	GLuint programID2 = LoadShaders("DebugVertexShader.vertexshader", "DebugFragmentShader.fragmentshader");
	mydebugdrawer.programID = programID2;
	if (debugmode) {
		dynamicsWorld->setDebugDrawer(&mydebugdrawer);
	}

	std::vector<btRigidBody*> rigidbodies;

	// Create a btTriangleIndexVertexArray
	btTriangleIndexVertexArray* targetData = new btTriangleIndexVertexArray;

	// Add an empty mesh (targetData makes a copy)
	btIndexedMesh tempMesh;
	targetData->addIndexedMesh(tempMesh, PHY_FLOAT);

	// Get a reference to internal copy of the empty mesh
	btIndexedMesh& mesh = targetData->getIndexedMeshArray()[0];

	// Allocate memory for the mesh
	const int32_t VERTICES_PER_TRIANGLE = 3;
	size_t numIndices = targetIndices.size();
	mesh.m_numTriangles = numIndices / VERTICES_PER_TRIANGLE;
	if (numIndices < std::numeric_limits<int16_t>::max()) {
		// we can use 16-bit indices
		mesh.m_triangleIndexBase = new unsigned char[sizeof(int16_t) * (size_t)numIndices];
		mesh.m_indexType = PHY_SHORT;
		mesh.m_triangleIndexStride = VERTICES_PER_TRIANGLE * sizeof(int16_t);
	}
	else {
		// we need 32-bit indices
		mesh.m_triangleIndexBase = new unsigned char[sizeof(int32_t) * (size_t)numIndices];
		mesh.m_indexType = PHY_INTEGER;
		mesh.m_triangleIndexStride = VERTICES_PER_TRIANGLE * sizeof(int32_t);
	}
	mesh.m_numVertices = targetVertices.size();
	mesh.m_vertexBase = new unsigned char[VERTICES_PER_TRIANGLE * sizeof(btScalar) * (size_t)mesh.m_numVertices];
	mesh.m_vertexStride = VERTICES_PER_TRIANGLE * sizeof(btScalar);

	// Copy vertices into mesh
	btScalar* vertexData = static_cast<btScalar*>((void*)(mesh.m_vertexBase));
	for (int32_t i = 0; i < mesh.m_numVertices; i++) {
		int32_t j = i * VERTICES_PER_TRIANGLE;
		const glm::vec3& point = targetVertices[i];
		vertexData[j] = point.x;
		vertexData[j + 1] = point.y;
		vertexData[j + 2] = point.z;
	}
	// Copy indices into mesh
	if (numIndices < std::numeric_limits<int16_t>::max()) {
		// 16-bit case
		int16_t* indices = static_cast<int16_t*>((void*)(mesh.m_triangleIndexBase));
		for (int32_t i = 0; i < numIndices; ++i) {
			indices[i] = (int16_t)targetIndices[i];
		}
	}
	else {
		// 32-bit case
		int32_t* indices = static_cast<int32_t*>((void*)(mesh.m_triangleIndexBase));
		for (int32_t i = 0; i < numIndices; ++i) {
			indices[i] = targetIndices[i];
		}
	}

	// Create the shape
	const bool USE_QUANTIZED_AABB_COMPRESSION = true;
	btBvhTriangleMeshShape* targetMeshShape = new btBvhTriangleMeshShape(targetData, USE_QUANTIZED_AABB_COMPRESSION);

	btCollisionShape* wallCollisionShape = new btBoxShape(btVector3(8.4f, 4.7f, 0.1f));

	btCollisionShape* floorCollisionShape = new btBoxShape(btVector3(8.4f, 0.1f, 8.3f));

	btCollisionShape* bulletCollisionShape = new btBoxShape(btVector3(0.05f, 0.1f, 0.05f));
	btScalar bulletMass = 0.00745f;
	btVector3 bulletInertia(0, 0, 0);
	bulletCollisionShape->calculateLocalInertia(bulletMass, bulletInertia);

	for (int i = 0; i < targetNum; i++) {

		btDefaultMotionState* targetMotionstate = new btDefaultMotionState(btTransform(
			btQuaternion(targetOrientations[i].x, targetOrientations[i].y, targetOrientations[i].z, targetOrientations[i].w),
			btVector3(targetPositions[i].x, targetPositions[i].y, targetPositions[i].z)
		));

		btRigidBody::btRigidBodyConstructionInfo targetRigidBodyCI(
			0,                  // mass, in kg. 0 -> Static object, will never move.
			targetMotionstate,
			targetMeshShape,  // collision shape of body
			btVector3(0, 0, 0)    // local inertia
		);
		btRigidBody *targetRigidBody = new btRigidBody(targetRigidBodyCI);

		rigidbodies.push_back(targetRigidBody);
		dynamicsWorld->addRigidBody(targetRigidBody);

		// Store the mesh's index "i" in Bullet's User index
		targetRigidBody->setUserIndex(i);
	}

	for (int i = 0; i < 6; i++) {

		btDefaultMotionState* wallMotionstate = new btDefaultMotionState(btTransform(
			btQuaternion(wallOrientations[i].x, wallOrientations[i].y, wallOrientations[i].z, wallOrientations[i].w),
			btVector3(wallPositions[i].x, wallPositions[i].y, wallPositions[i].z)
		));
		if (i < 4) {

			btRigidBody::btRigidBodyConstructionInfo wallRigidBodyCI(
				0,                  // mass, in kg. 0 -> Static object, will never move.
				wallMotionstate,
				wallCollisionShape,  // collision shape of body
				btVector3(0, 0, 0)    // local inertia
			);
			btRigidBody *wallRigidBody = new btRigidBody(wallRigidBodyCI);

			rigidbodies.push_back(wallRigidBody);
			dynamicsWorld->addRigidBody(wallRigidBody);

			// Store the mesh's index "i" in Bullet's User index
			wallRigidBody->setUserIndex(targetNum + i);
		}
		else {
			btRigidBody::btRigidBodyConstructionInfo wallRigidBodyCI(
				0,                  // mass, in kg. 0 -> Static object, will never move.
				wallMotionstate,
				floorCollisionShape,  // collision shape of body
				btVector3(0, 0, 0)    // local inertia
			);
			btRigidBody *wallRigidBody = new btRigidBody(wallRigidBodyCI);

			rigidbodies.push_back(wallRigidBody);
			dynamicsWorld->addRigidBody(wallRigidBody);

			// Store the mesh's index "i" in Bullet's User index
			wallRigidBody->setUserIndex(targetNum + i);
		}
	}

	bulletPositions[shots] = glm::vec3(getPosition().x + 0.3, getPosition().y, getPosition().z - 2);
	bulletRotations[shots] = glm::normalize(glm::quat(glm::vec3(1.5708 - getVertical(), 0.03054326 + getHorizontal(), 0)));

	btDefaultMotionState* bulletMotionstate = new btDefaultMotionState(btTransform(
		btQuaternion(bulletRotations[shots].x, bulletRotations[shots].y, bulletRotations[shots].z, bulletRotations[shots].w),
		btVector3(bulletPositions[shots].x, bulletPositions[shots].y, bulletPositions[shots].z)
	));

	btRigidBody::btRigidBodyConstructionInfo bulletRigidBodyCI(bulletMass, bulletMotionstate, bulletCollisionShape, bulletInertia);
	btRigidBody *bulletRigidBody = new btRigidBody(bulletRigidBodyCI);

	do {

		// Step the simulation at an interval of 60hz
		dynamicsWorld->stepSimulation(1 / 60.f, 10);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();

		if (debugmode) {
			mydebugdrawer.SetMatrices(ViewMatrix, ProjectionMatrix);
		}

		// Ray casting
		static int rcOldState = GLFW_RELEASE;
		int rcNewState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
		if (rcNewState == GLFW_RELEASE && rcOldState == GLFW_PRESS) {
			glm::vec3 out_origin;
			glm::vec3 out_direction;
			ScreenPosToWorldRay(
				windowWidth / 2, windowHeight / 2,
				windowWidth, windowHeight,
				ViewMatrix,
				ProjectionMatrix,
				out_origin,
				out_direction
			);

			glm::vec3 out_end = out_origin + out_direction*1000.0f;

			btCollisionWorld::ClosestRayResultCallback RayCallback(btVector3(out_origin.x, out_origin.y, out_origin.z), btVector3(out_end.x, out_end.y, out_end.z));
			dynamicsWorld->rayTest(btVector3(out_origin.x, out_origin.y, out_origin.z), btVector3(out_end.x, out_end.y, out_end.z), RayCallback);
			if (RayCallback.hasHit()) {
				if (RayCallback.m_collisionObject->getUserIndex() <= targetNum - 1) {
					printf("\nHit Target!");
				}
				else if (RayCallback.m_collisionObject->getUserIndex() > targetNum - 1) {
					printf("\nHit Wall!");
				}
			}
			else {
				printf("\nMiss!");
			}
		}
		rcOldState = rcNewState;

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use the shader
		glUseProgram(programID);

		////// Start render of room //////

		// Compute the MVP matrix from keyboard and mouse input
		glm::mat4 ModelMatrix1 = glm::mat4(1.0);
		glm::mat4 MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix1;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix1[0][0]);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, roomTexture);
		// Set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);

		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			roomIndices.size(),    // count
			GL_UNSIGNED_SHORT,   // type
			(void*)0           // element array buffer offset
		);

		////// End render of room //////
		////// Start render of bullet //////

		static int lcOldState = GLFW_RELEASE;
		int lcNewState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		if (lcNewState == GLFW_RELEASE && lcOldState == GLFW_PRESS && bulletCount > 0) {

			mat4 inverseViewMatrix = inverse(ViewMatrix);
			vec3 cameraPos(inverseViewMatrix[3]);

			glm::quat ViewRotation = (glm::quat)ViewMatrix;
			//glm::vec3 forward = (glm::vec3)(0, 0, 1);
			glm::vec3 normal = normalize(ViewRotation * cameraPos);
			std::cout << "\nRotation: " << glm::to_string(ViewRotation) << std::endl;
			std::cout << "Position: " << glm::to_string(cameraPos) << std::endl;
			std::cout << "Normal: " << glm::to_string(normal) << std::endl;

			// This should be the bullet's world-coordinates, right now it just draws infront of the player
			bulletPositions[shots] = glm::vec3(getPosition().x+0.3, getPosition().y, getPosition().z-2);
			// This should be the bullet's world-rotation, right now it just takes the camera's angles with an offset
			bulletRotations[shots] = glm::normalize(glm::quat(glm::vec3(1.5708-getVertical(), 0.03054326+getHorizontal(), 0)));

			btDefaultMotionState* bulletMotionstate = new btDefaultMotionState(btTransform(
				btQuaternion(bulletRotations[shots].x, bulletRotations[shots].y, bulletRotations[shots].z, bulletRotations[shots].w),
				btVector3(bulletPositions[shots].x, bulletPositions[shots].y, bulletPositions[shots].z)
			));

			btRigidBody::btRigidBodyConstructionInfo bulletRigidBodyCI(bulletMass, bulletMotionstate, bulletCollisionShape, bulletInertia);
			bulletRigidBody = new btRigidBody(bulletRigidBodyCI);

			rigidbodies.push_back(bulletRigidBody);
			dynamicsWorld->addRigidBody(bulletRigidBody);
			bulletRigidBodies[shots] = bulletRigidBody;

			bulletCount--;
			shots++;
		}
		lcOldState = lcNewState;

		for (int i = 0; i < shots; i++) {

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, bulletTexture);
			glUniform1i(TextureID, 0);

			// Rotate bullet based on camera angles
			glm::mat4 BulletRotationMatrix = glm::toMat4(bulletRotations[i]);
			// Translate it by getPosition(), the camera's x, y and z, with an offset
			glm::mat4 BulletTranslationMatrix = translate(mat4(), bulletPositions[i]);
			glm::mat4 BulletScaleMatrix = scale(mat4(), glm::vec3(0.1f, 0.1f, 0.1f));
			glm::mat4 ModelMatrix2 = BulletTranslationMatrix * BulletRotationMatrix * BulletScaleMatrix;
			glm::mat4 MVP2 = ProjectionMatrix * ViewMatrix * ModelMatrix2;
			//glm::mat4 BulletToWorld =  MVP2 * inverse(ViewMatrix);
			//glm::mat4 FinalMVP = ProjectionMatrix * ViewMatrix * BulletToWorld;

			//bulletPositions[i] = (glm::vec3) MVP2[3];	// Get the bullet's world coordinates

			//std::cout << glm::to_string(bulletCoordinates) << std::endl;

			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP2[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix2[0][0]);

			// 1st attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer2);
			glVertexAttribPointer(
				0,                  // attribute
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
			);

			// 2nd attribute buffer : UVs
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, uvbuffer2);
			glVertexAttribPointer(
				1,                                // attribute
				2,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
			);

			// 3rd attribute buffer : normals
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, normalbuffer2);
			glVertexAttribPointer(
				2,                                // attribute
				3,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
			);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer2);

			// Draw the triangles
			glDrawElements(GL_TRIANGLES, bulletIndices.size(), GL_UNSIGNED_SHORT, (void*)0);

			btTransform bulletTrans;
			bulletRigidBodies[i]->getMotionState()->getWorldTransform(bulletTrans);
			bulletPositions[i].x = bulletTrans.getOrigin().getX();
			bulletPositions[i].y = bulletTrans.getOrigin().getY();
			bulletPositions[i].z = bulletTrans.getOrigin().getZ();

			bulletRotations[i].x = bulletTrans.getRotation().getX();
			bulletRotations[i].y = bulletTrans.getRotation().getY();
			bulletRotations[i].z = bulletTrans.getRotation().getZ();
			bulletRotations[i].w = bulletTrans.getRotation().getW();
		}

		////// End render of bullet //////
		////// Start render of target //////

		for (int i = 0; i < targetNum; i++) {

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, targetTexture);
			glUniform1i(TextureID, 0);

			glm::mat4 TargetRotationMatrix = glm::toMat4(targetOrientations[i]);
			glm::mat4 TargetTranslationMatrix = translate(mat4(), targetPositions[i]);
			glm::mat4 ModelMatrix3 = TargetTranslationMatrix * TargetRotationMatrix;

			glm::mat4 MVP3 = ProjectionMatrix * ViewMatrix * ModelMatrix3;

			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP3[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix3[0][0]);

			// 1st attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer3);
			glVertexAttribPointer(
				0,                  // attribute
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
			);

			// 2nd attribute buffer : UVs
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, uvbuffer3);
			glVertexAttribPointer(
				1,                                // attribute
				2,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
			);

			// 3rd attribute buffer : normals
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, normalbuffer3);
			glVertexAttribPointer(
				2,                                // attribute
				3,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
			);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer3);

			// Draw the triangles
			glDrawElements(GL_TRIANGLES, targetIndices.size(), GL_UNSIGNED_SHORT, (void*)0);
		}

		////// End render of target //////
		////// Start render of gun //////

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gunTexture);
		glUniform1i(TextureID, 0);

		glm::mat4 ModelMatrix4 = glm::mat4(1.0);

		// Add an offset so the gun appears in view, rotate so that it's initially facing forward and scale it
		ModelMatrix4 = glm::translate(ModelMatrix4, glm::vec3(0.35, -0.85, -1.25));
		ModelMatrix4 = glm::scale(ModelMatrix4, glm::vec3(0.1f, 0.15f, 0.15f));
		ModelMatrix4 = glm::rotate(ModelMatrix4, 1.75f, glm::vec3(0.0f, 1.0f, 0.0f));

		glm::mat4 MVP4 = ProjectionMatrix * ModelMatrix4; // Left out * ViewMatrix

														  // Send our transformation to the currently bound shader, 
														  // in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP4[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix4[0][0]);

		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer4);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer4);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer4);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer4);

		// Draw the triangles
		glDrawElements(GL_TRIANGLES, gunIndices.size(), GL_UNSIGNED_SHORT, (void*)0);

		////// End render of gun //////

		// Debug draw Bullet scene
		if (debugmode) {
			glUseProgram(programID2);
			dynamicsWorld->debugDrawWorld();
		}

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	// First object
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);
	// Second object
	glDeleteBuffers(1, &vertexbuffer2);
	glDeleteBuffers(1, &uvbuffer2);
	glDeleteBuffers(1, &normalbuffer2);
	glDeleteBuffers(1, &elementbuffer2);
	// Third object
	glDeleteBuffers(1, &vertexbuffer3);
	glDeleteBuffers(1, &uvbuffer3);
	glDeleteBuffers(1, &normalbuffer3);
	glDeleteBuffers(1, &elementbuffer3);
	// Fourth object
	glDeleteBuffers(1, &vertexbuffer4);
	glDeleteBuffers(1, &uvbuffer4);
	glDeleteBuffers(1, &normalbuffer4);
	glDeleteBuffers(1, &elementbuffer4);
	glDeleteTextures(1, &TextureID);
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
	glfwDestroyCursor(crosshairCursor);

	// Clean up Bullet

	for (int i = 0; i < rigidbodies.size(); i++) {
		dynamicsWorld->removeRigidBody(rigidbodies[i]);
		delete rigidbodies[i]->getMotionState();
		delete rigidbodies[i];
	}
	delete targetMeshShape;
	delete wallCollisionShape;
	delete floorCollisionShape;
	delete bulletCollisionShape;
	delete targetData;
	delete dynamicsWorld;
	delete solver;
	delete collisionConfiguration;
	delete dispatcher;
	delete broadphase;

	return 0;
}