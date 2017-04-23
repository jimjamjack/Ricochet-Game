#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "Texture.h"

// Loads a .BMP image ONLY
GLuint loadTexture(const char* imagepath) {

	// Read data from .BMP file
	const int BMP_HEADER_SIZE = 54;
	unsigned char fileHeader[BMP_HEADER_SIZE];
	unsigned int imageSize;
	unsigned int imageWidth, imageHeight;
	unsigned char* imageData;

	// Open the file
	FILE* imageFile = fopen(imagepath, "rb");
	if (!imageFile) { 
		cout << "Image file could not be opened" << endl;
		return 0; 
	}

	// Read the header of the file
	// Check that the header is of the expected size
	if (fread(fileHeader, 1, 54, imageFile) != 54) {
		cout << "Incorrect BMP file" << endl;
		return 0;
	}
	// Check that the file begins with "B" and "M"
	if (fileHeader[0] != 'B' || fileHeader[1] != 'M') {
		cout << "Incorrect BMP file" << endl;
		return 0;
	}
	// Check that the file is 24BPP
	if (*(int*)&(fileHeader[30]) != 0) { 
		cout << "Incorrect BMP file" << endl;;
		return 0; 
	}
	if (*(int*)&(fileHeader[28]) != 24) {
		cout << "Incorrect BMP file" << endl;   
		return 0; 
	}

	// Read the image's size from the header
	imageSize = *(int*)&(fileHeader[34]);
	imageWidth = *(int*)&(fileHeader[18]);
	imageHeight = *(int*)&(fileHeader[22]);

	// Create a buffer for the actual image data
	imageData = new unsigned char[imageSize];

	// Read the actual data from the file into the buffer
	fread(imageData, 1, imageSize, imageFile);

	// Close file when done
	fclose(imageFile);

	// Create and bind the OpenGL texture before passing it to OpenGL
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, imageData);

	// Actual data no longer needed
	delete[] imageData;

	// Use trilinear filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Return the ID of the texture
	return textureID;
}