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

// Default game settings
bool debugMode = false;
bool slowMotion = false;
int targetNum = 10;
int bulletCount = 6;
float gravity = -9.81f;
float bulletPower = 100.0f;
float friction = 0.5f;
float bulletRestitution = 0.8f;

// Initialise variables
btDiscreteDynamicsWorld* dynamicsWorld;
int shots = 0;
int physicsBodyCount = 0;

// Used as a better structure for keeping track of all rigid bodies
struct physicsObject {
	int id;
	string type;
	bool hit;
	btRigidBody* body;
	physicsObject(btRigidBody* body, int id, string type) : body(body), id(id), type(type), hit(false) {}
};

// Target information
vector<vec3> targetPositions(targetNum);
vector<quat> targetOrientations(targetNum);
vector<btRigidBody*> targetRigidBodies(targetNum);

// Wall information
int wallNum = 6;
vector<vec3> wallPositions(wallNum);
vector<quat> wallOrientations(wallNum);

// Bullet information
vector<vec3> bulletPositions(bulletCount);
vector<quat> bulletOrientations(bulletCount);
vector<vec3> bulletLinePoints;
vector<btRigidBody*> bulletRigidBodies(bulletCount);

// Contains every rigid body in the scene and its information
vector<physicsObject*> gameObjects;

// Set up the game (default or custom)
void gameSetUp() {

	// The user's choices
	string useDefault;
	string useDebug;
	string enteredSlowMotion;
	int enteredNumber = 0;
	string enteredGravity;
	string enteredBulletPower;
	string enteredBulletType;
	string enteredSlipperyBullets;

	// Only change settings if user wants a custom game
	while (useDefault != "y" && useDefault != "n") {
		cout << "Would you like to use the default settings? (y/n): ";
		getline(cin, useDefault);
		if (useDefault == "n") {

			// Use debug mode?
			while (useDebug != "y" && useDebug != "n") {
				cout << "Would you like to use debug mode? (y/n): ";
				getline(cin, useDebug);
				if (useDebug == "y") {
					debugMode = true;
				}
			}

			// Use slow-motion?
			while (enteredSlowMotion != "y" && enteredSlowMotion != "n") {
				cout << "Would you like to use slow-motion? (y/n): ";
				getline(cin, enteredSlowMotion);
				if (enteredSlowMotion == "y") {
					slowMotion = true;
				}
			}

			// Number of targets?
			while (!(enteredNumber >= 3 && enteredNumber <= 15)) {
				cout << "How many targets would you like? (3 - 15): ";
				cin >> enteredNumber;
				while (cin.fail())
				{
					cout << "That was not an integer! Please enter an integer (3 - 15): ";
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					cin >> enteredNumber;
				}
				targetNum = enteredNumber;
				targetPositions.resize(targetNum);
				targetOrientations.resize(targetNum);
				targetRigidBodies.resize(targetNum);

				// Set number of bullets to less than the number of targets
				bulletCount = (int)floor(targetNum / 2) + 1;
				bulletPositions.resize(bulletCount);
				bulletOrientations.resize(bulletCount);
				bulletRigidBodies.resize(bulletCount);
				cin.ignore();
			}

			// Use which gravity?
			while (enteredGravity != "e" && enteredGravity != "m" && enteredGravity != "j") {
				cout << "Which gravity would you like? ((e)arth, (m)oon or (j)upiter): ";
				getline(cin, enteredGravity);
				if (enteredGravity == "m") {
					gravity = -1.6f;
				}
				else if (enteredGravity == "j") {
					gravity = -24.8f;
				}
			}

			// Use which bullet power?
			while (enteredBulletPower != "n" && enteredBulletPower != "s" && enteredBulletPower != "w") {
				cout << "Which level of power would you like your bullets to have? ((n)ormal, (s)trong or (w)eak): ";
				getline(cin, enteredBulletPower);
				if (enteredBulletPower == "s") {
					bulletPower = 200.0f;
				}
				else if (enteredBulletPower == "w") {
					bulletPower = 20.0f;
				}
			}

			// Use which bullet type?
			while (enteredBulletType != "n" && enteredBulletType != "r" && enteredBulletType != "p") {
				cout << "Which bullet type would you like? ((n)ormal, (r)ubber or (p)lastic): ";
				getline(cin, enteredBulletType);
				if (enteredBulletType == "r") {
					bulletRestitution = 1.0f;
				}
				else if (enteredBulletType == "p") {
					bulletRestitution = 0.1f;
				}
			}

			// Use slippery bullets?
			while (enteredSlipperyBullets != "y" && enteredSlipperyBullets != "n") {
				cout << "Would you like to have slippery bullets? (y/n): ";
				getline(cin, enteredSlipperyBullets);
				if (enteredSlipperyBullets == "y") {
					friction = 0.05f;
				}
			}
		}
	}
	// Seperate the HUD from the game set up
	cout << endl;
	cout << "-------------------------------HUD-------------------------------" << endl;
	cout << endl;
}

void initialiseGLFWWindow() {
	// Initialise GLFW
	if (!glfwInit())
	{
		cout << "Failed to initialize GLFW." << endl;
	}

	// Use 4x anti-aliasing
	glfwWindowHint(GLFW_SAMPLES, 4);

	// User correct context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	// Make window not resizable
	glfwWindowHint(GLFW_RESIZABLE, false);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Ricochet Game", NULL, NULL);
	if (window == NULL) {
		cout << "Failed to open GLFW window" << endl;
		glfwTerminate();
	}
	glfwMakeContextCurrent(window);

	// Initialise GLEW
	if (glewInit() != GLEW_OK) {
		cout << "Failed to initialize GLEW" << endl;
		glfwTerminate();
	}

	// Get window size
	int windowWidth, windowHeight;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	// Make sure that key presses are always captured
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Create a crosshair and use it as the default cursor
	GLFWcursor* crosshairCursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
	glfwSetCursor(window, crosshairCursor);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);

	// Use a basic grey background
	glClearColor(0.827f, 0.827f, 0.827f, 0.0f);

	// Enable depth test so drawn triangles don't overlap
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it's closer to the camera than the former one
	glDepthFunc(GL_LESS);
}

// Iterate through the contact manifold to find collisions
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
			// If there's a collision...
			if (contactPoint.getDistance() < 0.1f)
			{
				int rigidBodyIndexA = ((physicsObject*)objA->getUserPointer())->id;
				string rigidBodyTypeA = ((physicsObject*)objA->getUserPointer())->type;
				int rigidBodyIndexB = ((physicsObject*)objB->getUserPointer())->id;
				string rigidBodyTypeB = ((physicsObject*)objB->getUserPointer())->type;

				// Check if either of the colliding objects are targets
				if (rigidBodyTypeA == "Target" && gameObjects[rigidBodyIndexA]->hit == false) {
					gameObjects[rigidBodyIndexA]->hit = true;
				}

				if (rigidBodyTypeB == "Target" && gameObjects[rigidBodyIndexB]->hit == false) {
					gameObjects[rigidBodyIndexB]->hit = true;
				}
			}
		}
	}
}

// Randomly position and rotate targets so that they aren't overlapping
void generateTargets(vector<vec3> &positions, vector<quat> &orientations, int targetNum) {

	for (int i = 0; i < targetNum; i++) {
		int x = rand() % 12 - 5;
		int y = rand() % 5;
		int z = rand() % 12 - 5;
		// Check that the new position isn't too close to an existing one
		for (auto &position : positions) {
			while ((position.x <= x + 3) && (position.x >= x - 3) &&
				(position.y <= y + 3) && (position.y >= y - 3) &&
				(position.z <= z + 3) && (position.z >= z - 3)) {

				x = rand() % 12 - 5;
				y = rand() % 5;
				z = rand() % 12 - 5;
			}
		}
		positions[i] = vec3(x, y, z);
		orientations[i] = normalize(quat(vec3(rand() % 60, rand() % 60, rand() % 60)));
	}

}

// Generate the pre-set positions of the walls, floor and ceiling
void generateWalls(vector<vec3> &positions, vector<quat> &orientations) {

	positions[0] = vec3(1.02, 2.1, 9.5);
	orientations[0] = normalize(quat(vec3(0, 0, 0)));

	positions[1] = vec3(1.02, 2.1, -7.45);
	orientations[1] = normalize(quat(vec3(0, 0, 0)));

	positions[2] = vec3(-7.5, 2.1, 1.02);
	orientations[2] = normalize(quat(vec3(0, 1.5708, 0)));

	positions[3] = vec3(9.54, 2.1, 1.02);
	orientations[3] = normalize(quat(vec3(0, 1.5708, 0)));

	positions[4] = vec3(1.02, 6.9, 1.02);
	orientations[4] = normalize(quat(vec3(0, 0, 0)));

	positions[5] = vec3(1.02, -2.7, 1.02);
	orientations[5] = normalize(quat(vec3(0, 0, 0)));
}

// Load vertex data to the GPU
void VBOLoader(GLuint &indicesBuffer, GLuint &vertexBuffer, GLuint &uvBuffer, GLuint &normalBuffer, 
	vector<unsigned short> &indices, vector<vec3> &vertices, vector<vec2> &uvs, vector<vec3> &normals) {

	glGenBuffers(1, &indicesBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), &normals[0], GL_STATIC_DRAW);
}

// Update which targets are still active and keep count
void handleCounters() {

	// Each frame, update which targets have been hit
	int targetCount = 0;

	for (auto physicsObject : gameObjects) {
		if (physicsObject->type == "Target" && targetRigidBodies.at(physicsObject->id) != NULL) {
			targetCount++;
		}
		if (physicsObject->hit == true) {
			dynamicsWorld->removeRigidBody(gameObjects[physicsObject->id]->body);
			targetRigidBodies.at(physicsObject->id) = NULL;
		}
	}

	// Print the game's current time, bullet and target count
	string timeCounter = to_string(int(glfwGetTime()));
	string bulletCounter = string(2 - to_string(bulletCount).length(), '0') + to_string(bulletCount);
	string targetCounter = string(2 - to_string(targetCount).length(), '0') + to_string(targetCount);
	string counter = "Time Played: " + timeCounter + " Secs - Bullet Count: " + bulletCounter + " - Target Count: " + targetCounter;
	cout << counter;
	cout << string(counter.length(), '\b');
}

int main()
{

	// Initialise and set up game
	gameSetUp();
	initialiseGLFWWindow();

	// Get window size
	int windowWidth, windowHeight;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Load the model shaders
	GLuint programID = LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader");

	// Load the textures
	GLuint roomTexture = loadTexture("roomTexture.bmp");
	GLuint bulletTexture = loadTexture("bulletTexture.bmp");
	GLuint targetTexture = loadTexture("targetTexture.bmp");
	GLuint gunTexture = loadTexture("gunTexture.bmp");

	// Get a handle for the "textureSampler" uniform
	GLuint TextureID = glGetUniformLocation(programID, "textureSampler");

	// Get a handle for the "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Load the room model
	vector<unsigned short> roomIndices;
	vector<vec3> roomVertices;
	vector<vec2> roomUvs;
	vector<vec3> roomNormals;
	bool result = loadModel("room.obj", roomIndices, roomVertices, roomUvs, roomNormals);

	// Load the room vertex data into the GPU
	GLuint roomIndiceBuffer;
	GLuint roomVertexBuffer;
	GLuint roomUVBuffer;
	GLuint roomNormalBuffer;
	VBOLoader(roomIndiceBuffer, roomVertexBuffer, roomUVBuffer, roomNormalBuffer,
		roomIndices, roomVertices, roomUvs, roomNormals);

	// Load the bullet model
	vector<unsigned short> bulletIndices;
	vector<vec3> bulletVertices;
	vector<vec2> bulletUvs;
	vector<vec3> bulletNormals;
	result = loadModel("bullet.obj", bulletIndices, bulletVertices, bulletUvs, bulletNormals);

	// Load the bullet vertex data into the GPU
	GLuint bulletIndiceBuffer;
	GLuint bulletVertexBuffer;
	GLuint bulletUVBuffer;
	GLuint bulletNormalBuffer;
	VBOLoader(bulletIndiceBuffer, bulletVertexBuffer, bulletUVBuffer, bulletNormalBuffer,
		bulletIndices, bulletVertices, bulletUvs, bulletNormals);

	// Load the target model
	vector<unsigned short> targetIndices;
	vector<vec3> targetVertices;
	vector<vec2> targetUvs;
	vector<vec3> targetNormals;
	result = loadModel("target2.obj", targetIndices, targetVertices, targetUvs, targetNormals);

	// Load the target vertex data into the GPU
	GLuint targetIndiceBuffer;
	GLuint targetVertexBuffer;
	GLuint targetUVBuffer;
	GLuint targetNormalBuffer;
	VBOLoader(targetIndiceBuffer, targetVertexBuffer, targetUVBuffer, targetNormalBuffer,
		targetIndices, targetVertices, targetUvs, targetNormals);

	// Load the gun model
	vector<unsigned short> gunIndices;
	vector<vec3> gunVertices;
	vector<vec2> gunUvs;
	vector<vec3> gunNormals;
	result = loadModel("gun2.obj", gunIndices, gunVertices, gunUvs, gunNormals);

	// Load the gun vertex data into the GPU
	GLuint gunIndiceBuffer;
	GLuint gunVertexBuffer;
	GLuint gunUVBuffer;
	GLuint gunNormalBuffer;
	VBOLoader(gunIndiceBuffer, gunVertexBuffer, gunUVBuffer, gunNormalBuffer,
		gunIndices, gunVertices, gunUvs, gunNormals);

	// Seed the rand() function for more random results
	srand(unsigned int(time(NULL)));

	// Generate positions & rotations for n targets
	generateTargets(targetPositions, targetOrientations, targetNum);

	// Generate room surface positions & rotations
	generateWalls(wallPositions, wallOrientations);

	// Initialise Bullet

	// Create the broadphase
	btBroadphaseInterface* broadphase = new btDbvtBroadphase();

	// Create the collision configuration and dispatcher
	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);

	// Create the physics solver
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

	// Create the world
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, gravity, 0));

	// Use the collision callback method that checks for targets
	dynamicsWorld->setInternalTickCallback(contactTickCallback);

	// Use the debug drawer to draw rigid bodies
	BulletDebugDrawer_OpenGL customDebugDrawer;
	GLuint debugProgramID = LoadShaders("DebugVertexShader.vertexshader", "DebugFragmentShader.fragmentshader");
	customDebugDrawer.programID = debugProgramID;
	if (debugMode) {
		dynamicsWorld->setDebugDrawer(&customDebugDrawer);
	}

	// Create a btTriangleIndexVertexArray for the target's collision shape
	btTriangleIndexVertexArray* targetData = new btTriangleIndexVertexArray;

	// Add an empty mesh
	btIndexedMesh emptyMesh;
	targetData->addIndexedMesh(emptyMesh, PHY_FLOAT);

	// Get a reference to the empty mesh
	btIndexedMesh& targetMesh = targetData->getIndexedMeshArray()[0];

	// Allocate memory for the mesh
	const int32_t VERTICES_PER_TRIANGLE = 3;
	size_t numIndices = targetIndices.size();
	targetMesh.m_numTriangles = numIndices / VERTICES_PER_TRIANGLE;

	targetMesh.m_triangleIndexBase = new unsigned char[sizeof(int32_t) * (size_t)numIndices];
	targetMesh.m_indexType = PHY_INTEGER;
	targetMesh.m_triangleIndexStride = VERTICES_PER_TRIANGLE * sizeof(int32_t);

	targetMesh.m_numVertices = targetVertices.size();
	targetMesh.m_vertexBase = new unsigned char[VERTICES_PER_TRIANGLE * sizeof(btScalar) * (size_t)targetMesh.m_numVertices];
	targetMesh.m_vertexStride = VERTICES_PER_TRIANGLE * sizeof(btScalar);

	// Copy vertices into mesh
	btScalar* vertices = static_cast<btScalar*>((void*)(targetMesh.m_vertexBase));
	for (int32_t i = 0; i < targetMesh.m_numVertices; i++) {
		int32_t vertex = i * VERTICES_PER_TRIANGLE;
		const vec3& vertexPoint = targetVertices[i];
		vertices[vertex] = vertexPoint.x;
		vertices[vertex + 1] = vertexPoint.y;
		vertices[vertex + 2] = vertexPoint.z;
	}
	// Copy indices into mesh
	int32_t* indices = static_cast<int32_t*>((void*)(targetMesh.m_triangleIndexBase));
	for (int32_t i = 0; i < (int32_t)numIndices; i++) {
		indices[i] = (int32_t)targetIndices[i];
	}

	// Create target's mesh shape
	btBvhTriangleMeshShape* targetMeshShape = new btBvhTriangleMeshShape(targetData, true);

	// Create the collision shape for all of the walls
	btCollisionShape* wallCollisionShape = new btBoxShape(btVector3(8.4f, 4.7f, 0.1f));

	// Create collision shape for the floor and celing
	btCollisionShape* floorCollisionShape = new btBoxShape(btVector3(8.4f, 0.1f, 8.5f));

	// Create collision shape for the bullet and set its mass/inertia
	btCollisionShape* bulletCollisionShape = new btBoxShape(btVector3(0.03f, 0.04f, 0.03f));
	btScalar bulletMass = 0.00745f;
	btVector3 bulletInertia(0, 0, 0);
	bulletCollisionShape->calculateLocalInertia(bulletMass, bulletInertia);

	// Create all target rigid bodies
	for (int i = 0; i < targetNum; i++) {

		// Create each target's motion state given it's position and orientation
		btDefaultMotionState* targetMotionstate = new btDefaultMotionState(btTransform(
			btQuaternion(targetOrientations[i].x, targetOrientations[i].y, targetOrientations[i].z, targetOrientations[i].w),
			btVector3(targetPositions[i].x, targetPositions[i].y, targetPositions[i].z)
		));

		// Construct the actual rigid body
		btRigidBody::btRigidBodyConstructionInfo targetRigidBodyCI(0, targetMotionstate, targetMeshShape, btVector3(0, 0, 0));
		btRigidBody *targetRigidBody = new btRigidBody(targetRigidBodyCI);

		// Keep track of the rigid body and add it to the world
		gameObjects.push_back(new physicsObject(targetRigidBody, physicsBodyCount, "Target"));
		dynamicsWorld->addRigidBody(targetRigidBody);

		// Set how "bouncy" the object is
		targetRigidBody->setRestitution(1.0f);

		// Set the mesh's pointer add it to a collection of target rigid bodies
		targetRigidBody->setUserPointer(gameObjects[gameObjects.size()-1]);
		targetRigidBodies[i] = targetRigidBody;

		// Increment the total number of physics objects
		physicsBodyCount++;
	}

	// Create all wall rigid bodies
	for (int i = 0; i < 6; i++) {

		// Create each wall's motion state given it's position and orientation
		btDefaultMotionState* wallMotionstate = new btDefaultMotionState(btTransform(
			btQuaternion(wallOrientations[i].x, wallOrientations[i].y, wallOrientations[i].z, wallOrientations[i].w),
			btVector3(wallPositions[i].x, wallPositions[i].y, wallPositions[i].z)
		));
		// Walls are 0-4, floor and celing are 5 and 6 respectively
		if (i < 4) {

			// Construct the actual rigid body
			btRigidBody::btRigidBodyConstructionInfo wallRigidBodyCI(0, wallMotionstate, wallCollisionShape, btVector3(0, 0, 0));
			btRigidBody *wallRigidBody = new btRigidBody(wallRigidBodyCI);

			// Keep track of the rigid body and add it to the world
			gameObjects.push_back(new physicsObject(wallRigidBody, physicsBodyCount, "Wall"));
			dynamicsWorld->addRigidBody(wallRigidBody);

			// Set how "bouncy" the object is
			wallRigidBody->setRestitution(1.0f);

			// Set the mesh's pointer
			wallRigidBody->setUserPointer(gameObjects[gameObjects.size() - 1]);
		}
		else {

			// Construct the actual rigid body
			btRigidBody::btRigidBodyConstructionInfo wallRigidBodyCI(0, wallMotionstate, floorCollisionShape, btVector3(0, 0, 0));
			btRigidBody *wallRigidBody = new btRigidBody(wallRigidBodyCI);

			// Keep track of the rigid body and add it to the world
			gameObjects.push_back(new physicsObject(wallRigidBody, physicsBodyCount, "Wall"));
			dynamicsWorld->addRigidBody(wallRigidBody);

			// Set how "bouncy" the object is
			wallRigidBody->setRestitution(0.8f);

			// Set the mesh's pointer
			wallRigidBody->setUserPointer(gameObjects[gameObjects.size() - 1]);
		}

		// Increment the total number of physics objects
		physicsBodyCount++;
	}

	// Initialise bullet rigid body construction information
	bulletPositions[shots] = vec3(0, 0, 0);
	bulletOrientations[shots] = normalize(quat(vec3(0, 0, 0)));

	btDefaultMotionState* bulletMotionstate = new btDefaultMotionState(btTransform(
		btQuaternion(bulletOrientations[shots].x, bulletOrientations[shots].y, bulletOrientations[shots].z, bulletOrientations[shots].w),
		btVector3(bulletPositions[shots].x, bulletPositions[shots].y, bulletPositions[shots].z)
	));

	btRigidBody::btRigidBodyConstructionInfo bulletRigidBodyCI(bulletMass, bulletMotionstate, bulletCollisionShape, bulletInertia);
	btRigidBody *bulletRigidBody = new btRigidBody(bulletRigidBodyCI);

	// For calculating delta time
	double previousTime = glfwGetTime();

	do {

		// Get time difference between frames
		double currentTime = glfwGetTime();
		if (currentTime - previousTime >= 1.0) {
			previousTime += 1.0;
		}
		float deltaTime = (float)currentTime - (float)previousTime;

		// Use a slower simulation step?
		if (!slowMotion) {
			// Step the simulation according to delta time
			dynamicsWorld->stepSimulation(deltaTime, 7);
		}
		else {
			// Step the simulation according to delta time but much slower
			dynamicsWorld->stepSimulation(deltaTime / 1000.0f, 7);
		}

		// Update targets and counters
		handleCounters();

		// Update the camera's position and orientation
		updateCamera();
		mat4 ProjectionMatrix = getProjectionMatrix();
		mat4 ViewMatrix = getViewMatrix();

		// If debug mode is active, passing the required matrices to the debug drawer
		if (debugMode) {
			customDebugDrawer.SetMatrices(ViewMatrix, ProjectionMatrix);
		}

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use the shader programs
		glUseProgram(programID);

		/*-------------------------Rendering the room-------------------------*/

		// Bind the texture in Texture Unit 0 and use the texture sampler
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, roomTexture);
		glUniform1i(TextureID, 0);

		// Create the room model at the origin and transform into world-space
		mat4 RoomModelMatrix = mat4(1.0);
		mat4 RoomMVP = ProjectionMatrix * ViewMatrix * RoomModelMatrix;

		// Send the transformations to the shaders in the MVP uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &RoomMVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &RoomModelMatrix[0][0]);

		// Save vertex data to the Vertex Array Object (VAO)
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, roomVertexBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT , GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, roomUVBuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, roomNormalBuffer);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, roomIndiceBuffer);

		// Draw the triangles of the model
		glDrawElements(GL_TRIANGLES, roomIndices.size(), GL_UNSIGNED_SHORT, (void*)0);

		/*-------------------------End rendering the room-------------------------*/

		/*-------------------------Rendering the bullet path-------------------------*/

		// Store most current bullet's last position
		if (shots != 0) bulletLinePoints.push_back(vec3(bulletPositions[shots - 1].x, bulletPositions[shots - 1].y, bulletPositions[shots - 1].z));

		// Draw the bullet's path
		GLfloat thickness = 3.0;
		glLineWidth(thickness);
		glBegin(GL_LINE_STRIP);
		for (auto vec3 : bulletLinePoints) {
			glVertex3f(vec3.x, vec3.y, vec3.z);
		}
		glEnd();

		/*-------------------------End rendering the bullet path-------------------------*/

		/*-------------------------Rendering the bullets-------------------------*/

		// Get individual left clicks
		static int leftClickOldState = GLFW_RELEASE;
		int leftClickNewState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		if (leftClickNewState == GLFW_RELEASE && leftClickOldState == GLFW_PRESS && bulletCount > 0) {

			// Get the camera's direction and position from the view matrix
			mat4 inverseViewMatrix = inverse(ViewMatrix);
			vec3 cameraPosition = vec3(inverseViewMatrix[3]);
			vec3 cameraDirection = -vec3(inverseViewMatrix[2]);

			// Position of new bullet is always in front of the camera
			vec3 newBulletPosition = cameraPosition + cameraDirection;

			// Set the strength and velocity of each bullet
			vec3 bulletVelocity = cameraDirection * bulletPower;

			// offset bullet rotations so that they always fire from the gun
			float bulletOffsetVertical = 1.5708f;
			float bulletOffsetHorizontal = 0.03054326f;

			// Store each bullet's position and orientation
			bulletPositions[shots] = vec3(newBulletPosition.x, newBulletPosition.y, newBulletPosition.z);
			bulletOrientations[shots] = normalize(quat(vec3(bulletOffsetVertical-getVertical(), bulletOffsetHorizontal+getHorizontal(), 0)));

			// Create each bullet's motion state given it's position and orientation
			btDefaultMotionState* bulletMotionstate = new btDefaultMotionState(btTransform(
				btQuaternion(bulletOrientations[shots].x, bulletOrientations[shots].y, bulletOrientations[shots].z, bulletOrientations[shots].w),
				btVector3(bulletPositions[shots].x, bulletPositions[shots].y, bulletPositions[shots].z)
			));

			// Construct the actual rigid body
			btRigidBody::btRigidBodyConstructionInfo bulletRigidBodyCI(bulletMass, bulletMotionstate, bulletCollisionShape, bulletInertia);
			bulletRigidBody = new btRigidBody(bulletRigidBodyCI);

			// Keep track of the rigid body and add it to the world
			gameObjects.push_back(new physicsObject(bulletRigidBody, physicsBodyCount, "Bullet"));
			dynamicsWorld->addRigidBody(bulletRigidBody);

			// Set bullet's physics forces
			bulletRigidBody->setLinearVelocity(btVector3(bulletVelocity.x, bulletVelocity.y, bulletVelocity.z));
			bulletRigidBody->setRestitution(bulletRestitution);
			bulletRigidBody->setFriction(friction);

			// Enable continuous collision detection
			bulletRigidBody->setCcdMotionThreshold((btScalar)1e-7);
			bulletRigidBody->setCcdSweptSphereRadius((btScalar)0.03);

			// Set the mesh's pointer add it to a collection of bullet rigid bodies
			bulletRigidBody->setUserPointer(gameObjects[gameObjects.size() - 1]);
			bulletRigidBodies[shots] = bulletRigidBody;

			// Adjust relevant counters
			physicsBodyCount++;
			bulletCount--;
			shots++;

			bulletLinePoints.clear();
		}
		// Update state of left mouse button
		leftClickOldState = leftClickNewState;

		// Render every bullet that's been fired
		for (int i = 0; i < shots; i++) {

			// Bind the texture in Texture Unit 0 and use the texture sampler
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, bulletTexture);
			glUniform1i(TextureID, 0);

			// Rotate bullet based on camera orientation
			mat4 BulletRotationMatrix = toMat4(bulletOrientations[i]);
			// Position bullet at starting position
			mat4 BulletTranslationMatrix = translate(mat4(), bulletPositions[i]);
			// Shrink bullet to actual size
			mat4 BulletScaleMatrix = scale(mat4(), vec3(0.05f, 0.05f, 0.05f));

			// Transform into world-space
			mat4 BulletModelMatrix = BulletTranslationMatrix * BulletRotationMatrix * BulletScaleMatrix;
			mat4 BulletMVP2 = ProjectionMatrix * ViewMatrix * BulletModelMatrix;

			// Send the transformations to the shaders in the MVP uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &BulletMVP2[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &BulletModelMatrix[0][0]);

			// Save vertex data to the Vertex Array Object (VAO)
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, bulletVertexBuffer);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, bulletUVBuffer);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, bulletNormalBuffer);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bulletIndiceBuffer);

			// Draw the triangles of the model
			glDrawElements(GL_TRIANGLES, bulletIndices.size(), GL_UNSIGNED_SHORT, (void*)0);

			// Re-position and re-orientate bullet according to its rigid body
			btTransform bulletTransformation;
			bulletRigidBodies[i]->getMotionState()->getWorldTransform(bulletTransformation);

			bulletPositions[i].x = bulletTransformation.getOrigin().getX();
			bulletPositions[i].y = bulletTransformation.getOrigin().getY();
			bulletPositions[i].z = bulletTransformation.getOrigin().getZ();

			bulletOrientations[i].x = bulletTransformation.getRotation().getX();
			bulletOrientations[i].y = bulletTransformation.getRotation().getY();
			bulletOrientations[i].z = bulletTransformation.getRotation().getZ();
			bulletOrientations[i].w = bulletTransformation.getRotation().getW();
		}

		/*-------------------------End rendering the bullets-------------------------*/

		/*-------------------------Rendering the targets-------------------------*/

		int i = 0;

		// Render targets that have not been hit
		for (auto btRigidBody : targetRigidBodies) {
			if (btRigidBody != NULL) {

				// Bind the texture in Texture Unit 0 and use the texture sampler
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, targetTexture);
				glUniform1i(TextureID, 0);

				// Rotate target based on its randomly generated orientation
				mat4 TargetRotationMatrix = toMat4(targetOrientations[i]);
				// Position target based on its randomly generated position
				mat4 TargetTranslationMatrix = translate(mat4(), targetPositions[i]);

				// Transform into world-space
				mat4 TargetModelMatrix = TargetTranslationMatrix * TargetRotationMatrix;
				mat4 TargetMVP = ProjectionMatrix * ViewMatrix * TargetModelMatrix;

				// Send the transformations to the shaders in the MVP uniform
				glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &TargetMVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &TargetModelMatrix[0][0]);

				// Save vertex data to the Vertex Array Object (VAO)
				glEnableVertexAttribArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, targetVertexBuffer);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

				glEnableVertexAttribArray(1);
				glBindBuffer(GL_ARRAY_BUFFER, targetUVBuffer);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

				glEnableVertexAttribArray(2);
				glBindBuffer(GL_ARRAY_BUFFER, targetNormalBuffer);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, targetIndiceBuffer);

				// Draw the triangles of the model
				glDrawElements(GL_TRIANGLES, targetIndices.size(), GL_UNSIGNED_SHORT, (void*)0);
			}
			i++;
		}

		/*-------------------------End rendering the targets-------------------------*/

		/*-------------------------Rendering the gun-------------------------*/

		// Bind the texture in Texture Unit 0 and use the texture sampler
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gunTexture);
		glUniform1i(TextureID, 0);

		// Add an offset so the gun appears in view
		mat4 GunTranslationMatrix = translate(mat4(), vec3(0.35, -0.85, -1.25));
		// Rotate the gun so that it's initially facing forward
		mat4 GunRotationMatrix = toMat4(normalize(quat(vec3(0, 90, 0))));
		// Scale the gun down to the correct size
		mat4 GunScaleMatrix = scale(mat4(), vec3(0.1f, 0.15f, 0.15f));

		// Transform into view-space
		mat4 GunModelMatrix = GunTranslationMatrix * GunRotationMatrix * GunScaleMatrix;
		mat4 GunMVP = ProjectionMatrix * GunModelMatrix;

		// Send the transformations to the shaders in the MVP uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &GunMVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &GunModelMatrix[0][0]);

		// Save vertex data to the Vertex Array Object (VAO)
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, gunVertexBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, gunUVBuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, gunNormalBuffer);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gunIndiceBuffer);

		// Draw the triangles of the model
		glDrawElements(GL_TRIANGLES, gunIndices.size(), GL_UNSIGNED_SHORT, (void*)0);

		/*-------------------------End rendering the gun-------------------------*/

		// Have the debug drawer draw the world
		if (debugMode) {
			glUseProgram(debugProgramID);
			dynamicsWorld->debugDrawWorld();
		}

		// Clean up VAO
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Clean up VBO
	// Room object
	glDeleteBuffers(1, &roomVertexBuffer);
	glDeleteBuffers(1, &roomUVBuffer);
	glDeleteBuffers(1, &roomNormalBuffer);
	glDeleteBuffers(1, &roomIndiceBuffer);
	// Bullet object
	glDeleteBuffers(1, &bulletVertexBuffer);
	glDeleteBuffers(1, &bulletUVBuffer);
	glDeleteBuffers(1, &bulletNormalBuffer);
	glDeleteBuffers(1, &bulletIndiceBuffer);
	// Target object
	glDeleteBuffers(1, &targetVertexBuffer);
	glDeleteBuffers(1, &targetUVBuffer);
	glDeleteBuffers(1, &targetNormalBuffer);
	glDeleteBuffers(1, &targetIndiceBuffer);
	// Gun object
	glDeleteBuffers(1, &gunVertexBuffer);
	glDeleteBuffers(1, &gunUVBuffer);
	glDeleteBuffers(1, &gunNormalBuffer);
	glDeleteBuffers(1, &gunIndiceBuffer);

	// Clean up textures and shaders
	glDeleteTextures(1, &TextureID);
	glDeleteProgram(programID);
	glDeleteProgram(debugProgramID);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close window and terminate GLFW
	glfwTerminate();

	// Clean up Bullet
	for (size_t i = 0; i < gameObjects.size(); i++) {
		dynamicsWorld->removeRigidBody(gameObjects[i]->body);
		delete gameObjects[i]->body->getMotionState();
		delete gameObjects[i]->body;
		delete gameObjects[i];
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