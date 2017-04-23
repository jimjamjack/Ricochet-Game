#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace std;

#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>

#include "Shader.h"

// Load the vertex and fragment shaders
GLuint LoadShaders(const char* vertexFilePath, const char* fragmentFilePath){

	// Create each of the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read in the code for the vertex shader
	string vertexShader;
	ifstream vInputStream(vertexFilePath, ios::in);
	string nextLine = "";
	if (vInputStream.is_open()) {
		while (getline(vInputStream, nextLine))
			vertexShader += "\n" + nextLine;
		vInputStream.close();
	}
	else {
		cout << "Error when opening vertex shader" << endl;
		return 0;
	}

	// Read in the code for the fragment shader
	string fragmentShader;
	ifstream fInputStream(fragmentFilePath, ios::in);
	if (fInputStream.is_open()) {
		while (getline(fInputStream, nextLine))
			fragmentShader += "\n" + nextLine;
		fInputStream.close();
	}
	else {
		cout << "Error when opening fragment shader" << endl;
		return 0;
	}

	// Compile vertex shader with a pointer to the contents of the shader code
	char const* vertexShaderPointer = vertexShader.c_str();
	glShaderSource(VertexShaderID, 1, &vertexShaderPointer, NULL);
	glCompileShader(VertexShaderID);

	// Compile fragment shader with a pointer to the contents of the shader code
	char const* fragmentShaderPointer = fragmentShader.c_str();
	glShaderSource(FragmentShaderID, 1, &fragmentShaderPointer, NULL);
	glCompileShader(FragmentShaderID);

	// Link the shaders to OpenGL
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}