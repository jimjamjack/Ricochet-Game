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
#include <btBulletCollisionCommon.h>
#include <LinearMath/btIDebugDraw.h>

// Include my files
#include "Shader.h"
#include "Camera.h"
#include "ObjLoader.h"
#include "Texture.h"
#include "DebugDrawer.h"

bool debugmode = true; // False
int targetNum = 15; // 10
int bulletCount = 15; // 10/2
int shots = 0;
int gravity = -9.81f;
btScalar desiredVelocity;
btDiscreteDynamicsWorld* dynamicsWorld;
int userIndexNum = 0;

struct physicsObject {
	int id;
	string type;
	bool hit;
	btRigidBody* body;
	physicsObject(btRigidBody* body, int id, string type) : body(body), id(id), type(type), hit(false) {}
};

std::vector<glm::vec3> targetPositions(targetNum);
std::vector<glm::quat> targetOrientations(targetNum);
std::vector<btRigidBody*> targetRigidBodies(targetNum);

int wallNum = 6;
std::vector<glm::vec3> wallPositions(wallNum);
std::vector<glm::quat> wallOrientations(wallNum);

std::vector<glm::vec3> bulletPositions(bulletCount);
std::vector<glm::quat> bulletRotations(bulletCount);
std::vector<btRigidBody*> bulletRigidBodies(bulletCount);

std::vector<physicsObject*> rigidbodies;

void gameSetUp() {
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
				targetPositions.resize(targetNum);
				targetOrientations.resize(targetNum);
				targetRigidBodies.resize(targetNum);

				bulletCount = floor(targetNum / 2);
				bulletPositions.resize(bulletCount);
				bulletRotations.resize(bulletCount);
				bulletRigidBodies.resize(bulletCount);
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
					gravity = -24.8f;
				}
			}
		}
	}
}

void initialiseGLFWWindow() {
	// Initialise GLFW
	if (!glfwInit())
	{
		printf("Failed to initialize GLFW.\n");
		getchar();
	}

	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, false);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Ricochet Game", NULL, NULL);
	if (window == NULL) {
		cout << "Failed to open GLFW window" << endl;
		glfwTerminate();
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		cout << "Failed to initialize GLEW" << endl;
		glfwTerminate();
	}

	int windowWidth, windowHeight;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	// Ensure we can capture the escape key being pressed (key remains in GLFW_PRESS state until glfwGetKey)
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Create a crosshair and use it as the default cursor
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	GLFWcursor* crosshairCursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
	glfwSetCursor(window, crosshairCursor);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);

	// Basic grey background
	glClearColor(0.827f, 0.827f, 0.827f, 0.0f);

	// Enable depth test so drawn triangles don't overlap
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it's closer to the camera than the former one
	glDepthFunc(GL_LESS);
}

void contactTickCallback(btDynamicsWorld *dynamicsWorld, btScalar timeStep) {
	int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++)
	{
		btPersistentManifold* contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
		const btCollisionObject *objA = contactManifold->getBody0();
		const btCollisionObject *objB = contactManifold->getBody1();

		int numContacts = contactManifold->getNumContacts();
		for (int j = 0; j < numContacts; j++)
		{
			btManifoldPoint& contactPoint = contactManifold->getContactPoint(j);
			if (contactPoint.getDistance() < 0.1f)
			{
				int rigidBodyIndexA = ((physicsObject*)objA->getUserPointer())->id;
				string rigidBodyTypeA = ((physicsObject*)objA->getUserPointer())->type;
				int rigidBodyIndexB = ((physicsObject*)objB->getUserPointer())->id;
				string rigidBodyTypeB = ((physicsObject*)objB->getUserPointer())->type;

				if (rigidBodyTypeA == "Target" && rigidbodies[rigidBodyIndexA]->hit == false) {
					rigidbodies[rigidBodyIndexA]->hit = true;
				}

				if (rigidBodyTypeB == "Target" && rigidbodies[rigidBodyIndexB]->hit == false) {
					rigidbodies[rigidBodyIndexB]->hit = true;
				}
			}
		}
	}
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

void VBOLoader(GLuint &indicesBuffer, GLuint &vertexBuffer, GLuint &uvBuffer, GLuint &normalBuffer, 
	std::vector<unsigned short> &indices, std::vector<glm::vec3> &vertices, std::vector<glm::vec2> &uvs, std::vector<glm::vec3> &normals) {

	glGenBuffers(1, &indicesBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
}

void handleCounters() {
	int targetCount = 0;

	for (auto physicsObject : rigidbodies) {
		if (physicsObject->type == "Target" && targetRigidBodies.at(physicsObject->id) != NULL) {
			targetCount++;
		}
		if (physicsObject->hit == true) {
			dynamicsWorld->removeRigidBody(rigidbodies[physicsObject->id]->body);
			targetRigidBodies.at(physicsObject->id) = NULL;
		}
	}

	string bulletCounter = string(2 - to_string(bulletCount).length(), '0') + to_string(bulletCount);
	string targetCounter = string(2 - to_string(targetCount).length(), '0') + to_string(targetCount);
	string counter = "Bullet Count: " + bulletCounter + " - Target Count: " + targetCounter;
	cout << counter;
	cout << string(counter.length(), '\b');
}

int main()
{

	gameSetUp();
	initialiseGLFWWindow();

	int windowWidth, windowHeight;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile the GLSL program from the shaders
	GLuint programID = LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader");

	// Load the textures
	GLuint roomTexture = loadTexture("roomTexture.bmp");
	GLuint bulletTexture = loadTexture("bulletTexture.bmp");
	GLuint targetTexture = loadTexture("targetTexture.bmp");
	GLuint gunTexture = loadTexture("gunTexture.bmp");

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

	GLuint roomIndiceBuffer;
	GLuint roomVertexBuffer;
	GLuint roomUVBuffer;
	GLuint roomNormalBuffer;
	VBOLoader(roomIndiceBuffer, roomVertexBuffer, roomUVBuffer, roomNormalBuffer,
		roomIndices, roomVertices, roomUvs, roomNormals);

	std::vector<unsigned short> bulletIndices;
	std::vector<glm::vec3> bulletVertices;
	std::vector<glm::vec2> bulletUvs;
	std::vector<glm::vec3> bulletNormals;
	res = loadAssImp("bullet.obj", bulletIndices, bulletVertices, bulletUvs, bulletNormals);

	GLuint bulletIndiceBuffer;
	GLuint bulletVertexBuffer;
	GLuint bulletUVBuffer;
	GLuint bulletNormalBuffer;
	VBOLoader(bulletIndiceBuffer, bulletVertexBuffer, bulletUVBuffer, bulletNormalBuffer,
		bulletIndices, bulletVertices, bulletUvs, bulletNormals);

	std::vector<unsigned short> targetIndices;
	std::vector<glm::vec3> targetVertices;
	std::vector<glm::vec2> targetUvs;
	std::vector<glm::vec3> targetNormals;
	res = loadAssImp("target2.obj", targetIndices, targetVertices, targetUvs, targetNormals);

	GLuint targetIndiceBuffer;
	GLuint targetVertexBuffer;
	GLuint targetUVBuffer;
	GLuint targetNormalBuffer;
	VBOLoader(targetIndiceBuffer, targetVertexBuffer, targetUVBuffer, targetNormalBuffer,
		targetIndices, targetVertices, targetUvs, targetNormals);

	std::vector<unsigned short> gunIndices;
	std::vector<glm::vec3> gunVertices;
	std::vector<glm::vec2> gunUvs;
	std::vector<glm::vec3> gunNormals;
	res = loadAssImp("gun2.obj", gunIndices, gunVertices, gunUvs, gunNormals);

	GLuint gunIndiceBuffer;
	GLuint gunVertexBuffer;
	GLuint gunUVBuffer;
	GLuint gunNormalBuffer;
	VBOLoader(gunIndiceBuffer, gunVertexBuffer, gunUVBuffer, gunNormalBuffer,
		gunIndices, gunVertices, gunUvs, gunNormals);

	// Seed the rand() function for more random results
	srand(time(NULL));

	// Generate positions & rotations for n targets
	generateTargets(targetPositions, targetOrientations, targetNum);

	// Generate room surface positions & rotations
	generateWalls(wallPositions, wallOrientations);

	// Initialize Bullet

	// Build the broadphase
	btBroadphaseInterface* broadphase = new btDbvtBroadphase();

	// Set up the collision configuration and dispatcher
	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);

	// The actual physics solver
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

	// The world
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, gravity, 0));

	dynamicsWorld->setInternalTickCallback(contactTickCallback);

	BulletDebugDrawer_OpenGL mydebugdrawer;
	GLuint programID2 = LoadShaders("DebugVertexShader.vertexshader", "DebugFragmentShader.fragmentshader");
	mydebugdrawer.programID = programID2;
	if (debugmode) {
		dynamicsWorld->setDebugDrawer(&mydebugdrawer);
	}

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

	mesh.m_triangleIndexBase = new unsigned char[sizeof(int32_t) * (size_t)numIndices];
	mesh.m_indexType = PHY_INTEGER;
	mesh.m_triangleIndexStride = VERTICES_PER_TRIANGLE * sizeof(int32_t);

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
	int32_t* indices = static_cast<int32_t*>((void*)(mesh.m_triangleIndexBase));
	for (int32_t i = 0; i < numIndices; ++i) {
		indices[i] = (int32_t)targetIndices[i];
	}

	// Create the shape
	btBvhTriangleMeshShape* targetMeshShape = new btBvhTriangleMeshShape(targetData, true);

	btCollisionShape* wallCollisionShape = new btBoxShape(btVector3(8.4f, 4.7f, 0.1f));

	btCollisionShape* floorCollisionShape = new btBoxShape(btVector3(8.4f, 0.1f, 8.5f));

	btCollisionShape* bulletCollisionShape = new btBoxShape(btVector3(0.03f, 0.04f, 0.03f));
	btScalar bulletMass = 0.00745f;
	btVector3 bulletInertia(0, 0, 0);
	bulletCollisionShape->calculateLocalInertia(bulletMass, bulletInertia);

	for (int i = 0; i < targetNum; i++) {

		btDefaultMotionState* targetMotionstate = new btDefaultMotionState(btTransform(
			btQuaternion(targetOrientations[i].x, targetOrientations[i].y, targetOrientations[i].z, targetOrientations[i].w),
			btVector3(targetPositions[i].x, targetPositions[i].y, targetPositions[i].z)
		));

		btRigidBody::btRigidBodyConstructionInfo targetRigidBodyCI(0, targetMotionstate, targetMeshShape, btVector3(0, 0, 0));
		btRigidBody *targetRigidBody = new btRigidBody(targetRigidBodyCI);

		rigidbodies.push_back(new physicsObject(targetRigidBody, userIndexNum, "Target"));
		dynamicsWorld->addRigidBody(targetRigidBody);
		targetRigidBody->setRestitution(0.9);

		// Store the mesh's index "i" in Bullet's User index
		targetRigidBody->setUserPointer(rigidbodies[rigidbodies.size()-1]);

		targetRigidBodies[i] = targetRigidBody;

		userIndexNum++;
	}

	for (int i = 0; i < 6; i++) {

		btDefaultMotionState* wallMotionstate = new btDefaultMotionState(btTransform(
			btQuaternion(wallOrientations[i].x, wallOrientations[i].y, wallOrientations[i].z, wallOrientations[i].w),
			btVector3(wallPositions[i].x, wallPositions[i].y, wallPositions[i].z)
		));
		if (i < 4) {

			btRigidBody::btRigidBodyConstructionInfo wallRigidBodyCI(0, wallMotionstate, wallCollisionShape, btVector3(0, 0, 0));
			btRigidBody *wallRigidBody = new btRigidBody(wallRigidBodyCI);

			rigidbodies.push_back(new physicsObject(wallRigidBody, userIndexNum, "Wall"));
			dynamicsWorld->addRigidBody(wallRigidBody);

			// Store the mesh's index "i" in Bullet's User index
			wallRigidBody->setUserPointer(rigidbodies[rigidbodies.size() - 1]);

			wallRigidBody->setRestitution(0.9);
		}
		else {
			btRigidBody::btRigidBodyConstructionInfo wallRigidBodyCI(0, wallMotionstate, floorCollisionShape, btVector3(0, 0, 0));
			btRigidBody *wallRigidBody = new btRigidBody(wallRigidBodyCI);

			rigidbodies.push_back(new physicsObject(wallRigidBody, userIndexNum, "Wall"));
			dynamicsWorld->addRigidBody(wallRigidBody);

			// Store the mesh's index "i" in Bullet's User index
			wallRigidBody->setUserPointer(rigidbodies[rigidbodies.size() - 1]);

			wallRigidBody->setRestitution(0.9);
		}
		userIndexNum++;
	}

	bulletPositions[shots] = glm::vec3(0, 0, 0);
	bulletRotations[shots] = glm::normalize(glm::quat(glm::vec3(0, 0, 0)));

	btDefaultMotionState* bulletMotionstate = new btDefaultMotionState(btTransform(
		btQuaternion(bulletRotations[shots].x, bulletRotations[shots].y, bulletRotations[shots].z, bulletRotations[shots].w),
		btVector3(bulletPositions[shots].x, bulletPositions[shots].y, bulletPositions[shots].z)
	));

	btRigidBody::btRigidBodyConstructionInfo bulletRigidBodyCI(bulletMass, bulletMotionstate, bulletCollisionShape, bulletInertia);
	btRigidBody *bulletRigidBody = new btRigidBody(bulletRigidBodyCI);

	// For speed computation
	double lastTime = glfwGetTime();

	do {

		// Measure speed
		double currentTime = glfwGetTime();
		if (currentTime - lastTime >= 1.0) {
			lastTime += 1.0;
		}
		float deltaTime = currentTime - lastTime;

		dynamicsWorld->stepSimulation(deltaTime, 7);
		//// Step the simulation at an interval of 60hz
		//dynamicsWorld->stepSimulation(1 / 60.f, 10);

		handleCounters();

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();

		if (debugmode) {
			mydebugdrawer.SetMatrices(ViewMatrix, ProjectionMatrix);
		}

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use the shader
		glUseProgram(programID);

		////// Start render of room //////

		// Compute the MVP matrix from keyboard and mouse input
		glm::mat4 RoomModelMatrix = glm::mat4(1.0);
		glm::mat4 RoomMVP = ProjectionMatrix * ViewMatrix * RoomModelMatrix;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &RoomMVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &RoomModelMatrix[0][0]);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, roomTexture);
		// Set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);

		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, roomVertexBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT , GL_FALSE, 0, (void*)0);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, roomUVBuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, roomNormalBuffer);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, roomIndiceBuffer);

		// Draw the triangles
		glDrawElements(GL_TRIANGLES, roomIndices.size(), GL_UNSIGNED_SHORT, (void*)0);

		////// End render of room //////
		////// Start render of bullet //////

		static int lcOldState = GLFW_RELEASE;
		int lcNewState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		if (lcNewState == GLFW_RELEASE && lcOldState == GLFW_PRESS && bulletCount > 0) {

			mat4 inverseViewMatrix = inverse(ViewMatrix);
			vec3 cameraPosition = vec3(inverseViewMatrix[3]);
			vec3 cameraDirection = -vec3(inverseViewMatrix[2]);
			vec3 newBulletPosition = cameraPosition + cameraDirection;

			float bulletPower = 100.0f;
			glm::vec3 bulletVelocity = cameraDirection * bulletPower;


			float bulletOffsetVertical = 1.5708;
			float bulletOffsetHorizontal = 0.03054326;
			bulletPositions[shots] = glm::vec3(newBulletPosition.x, newBulletPosition.y, newBulletPosition.z);
			bulletRotations[shots] = glm::normalize(glm::quat(glm::vec3(bulletOffsetVertical-getVertical(), bulletOffsetHorizontal+getHorizontal(), 0)));

			btDefaultMotionState* bulletMotionstate = new btDefaultMotionState(btTransform(
				btQuaternion(bulletRotations[shots].x, bulletRotations[shots].y, bulletRotations[shots].z, bulletRotations[shots].w),
				btVector3(bulletPositions[shots].x, bulletPositions[shots].y, bulletPositions[shots].z)
			));

			btRigidBody::btRigidBodyConstructionInfo bulletRigidBodyCI(bulletMass, bulletMotionstate, bulletCollisionShape, bulletInertia);
			bulletRigidBody = new btRigidBody(bulletRigidBodyCI);

			rigidbodies.push_back(new physicsObject(bulletRigidBody, userIndexNum, "Bullet"));
			dynamicsWorld->addRigidBody(bulletRigidBody);
			// Store the mesh's index "i" in Bullet's User index
			bulletRigidBody->setUserPointer(rigidbodies[rigidbodies.size() - 1]);
			userIndexNum++;

			bulletRigidBodies[shots] = bulletRigidBody;
			bulletRigidBody->setLinearVelocity(btVector3(bulletVelocity.x, bulletVelocity.y, bulletVelocity.z));
			desiredVelocity = bulletRigidBody->getLinearVelocity().length();
			bulletRigidBody->setRestitution(0.5);
			//bulletRigidBody->setFriction(0);

			// Check these settings
			bulletRigidBody->setCcdMotionThreshold(1e-7);
			bulletRigidBody->setCcdSweptSphereRadius(0.015);

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
			// Offset so that it appears after the barrel of the gun
			glm::mat4 BulletTranslationMatrix = translate(mat4(), bulletPositions[i]);
			glm::mat4 BulletScaleMatrix = scale(mat4(), glm::vec3(0.05f, 0.05f, 0.05f));
			glm::mat4 BulletModelMatrix = BulletTranslationMatrix * BulletRotationMatrix * BulletScaleMatrix;
			glm::mat4 BulletMVP2 = ProjectionMatrix * ViewMatrix * BulletModelMatrix;

			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &BulletMVP2[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &BulletModelMatrix[0][0]);

			// 1st attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, bulletVertexBuffer);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			// 2nd attribute buffer : UVs
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, bulletUVBuffer);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

			// 3rd attribute buffer : normals
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, bulletNormalBuffer);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bulletIndiceBuffer);

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

		int i = 0;

		for (auto btRigidBody : targetRigidBodies) {
			if (btRigidBody != NULL) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, targetTexture);
				glUniform1i(TextureID, 0);

				glm::mat4 TargetRotationMatrix = glm::toMat4(targetOrientations[i]);
				glm::mat4 TargetTranslationMatrix = translate(mat4(), targetPositions[i]);
				glm::mat4 TargetModelMatrix = TargetTranslationMatrix * TargetRotationMatrix;

				glm::mat4 TargetMVP = ProjectionMatrix * ViewMatrix * TargetModelMatrix;

				// Send our transformation to the currently bound shader, 
				// in the "MVP" uniform
				glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &TargetMVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &TargetModelMatrix[0][0]);

				// 1st attribute buffer : vertices
				glEnableVertexAttribArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, targetVertexBuffer);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

				// 2nd attribute buffer : UVs
				glEnableVertexAttribArray(1);
				glBindBuffer(GL_ARRAY_BUFFER, targetUVBuffer);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

				// 3rd attribute buffer : normals
				glEnableVertexAttribArray(2);
				glBindBuffer(GL_ARRAY_BUFFER, targetNormalBuffer);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

				// Index buffer
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, targetIndiceBuffer);

				// Draw the triangles
				glDrawElements(GL_TRIANGLES, targetIndices.size(), GL_UNSIGNED_SHORT, (void*)0);
			}
			i++;
		}

		////// End render of target //////
		////// Start render of gun //////

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gunTexture);
		glUniform1i(TextureID, 0);

		glm::mat4 GunModelMatrix = glm::mat4(1.0);

		// Add an offset so the gun appears in view, rotate so that it's initially facing forward and scale it
		GunModelMatrix = glm::translate(GunModelMatrix, glm::vec3(0.35, -0.85, -1.25));
		GunModelMatrix = glm::scale(GunModelMatrix, glm::vec3(0.1f, 0.15f, 0.15f));
		GunModelMatrix = glm::rotate(GunModelMatrix, 1.75f, glm::vec3(0.0f, 1.0f, 0.0f));

		glm::mat4 GunMVP = ProjectionMatrix * GunModelMatrix;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &GunMVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &GunModelMatrix[0][0]);

		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, gunVertexBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, gunUVBuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, gunNormalBuffer);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gunIndiceBuffer);

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
	glDeleteBuffers(1, &roomVertexBuffer);
	glDeleteBuffers(1, &roomUVBuffer);
	glDeleteBuffers(1, &roomNormalBuffer);
	glDeleteBuffers(1, &roomIndiceBuffer);
	// Second object
	glDeleteBuffers(1, &bulletVertexBuffer);
	glDeleteBuffers(1, &bulletUVBuffer);
	glDeleteBuffers(1, &bulletNormalBuffer);
	glDeleteBuffers(1, &bulletIndiceBuffer);
	// Third object
	glDeleteBuffers(1, &targetVertexBuffer);
	glDeleteBuffers(1, &targetUVBuffer);
	glDeleteBuffers(1, &targetNormalBuffer);
	glDeleteBuffers(1, &targetIndiceBuffer);
	// Fourth object
	glDeleteBuffers(1, &gunVertexBuffer);
	glDeleteBuffers(1, &gunUVBuffer);
	glDeleteBuffers(1, &gunNormalBuffer);
	glDeleteBuffers(1, &gunIndiceBuffer);
	glDeleteTextures(1, &TextureID);
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	// Clean up Bullet
	for (int i = 0; i < rigidbodies.size(); i++) {
		dynamicsWorld->removeRigidBody(rigidbodies[i]->body);
		delete rigidbodies[i]->body->getMotionState();
		delete rigidbodies[i]->body;
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