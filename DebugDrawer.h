#ifndef DEBUGDRAWER_H
#define DEBUGDRAWER_H

// Include GLEW
#include <GL/glew.h>

// Include DebugDrawInterface
#include <LinearMath/btIDebugDraw.h>


GLuint VBO, VAO;
class BulletDebugDrawer_OpenGL : public btIDebugDraw {
public:

	GLuint programID;

	void SetMatrices(glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
	{
		glUniformMatrix4fv(glGetUniformLocation(programID, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
		glUniformMatrix4fv(glGetUniformLocation(programID, "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
	}

	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& colour)
	{
		// Vertex data
		GLfloat vertices[12];

		vertices[0] = from.x();
		vertices[1] = from.y();
		vertices[2] = from.z();
		vertices[3] = colour.x();
		vertices[4] = colour.y();
		vertices[5] = colour.z();
		vertices[6] = to.x();
		vertices[7] = to.y();
		vertices[8] = to.z();
		vertices[9] = colour.x();
		vertices[10] = colour.y();
		vertices[11] = colour.z();

		glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glBindVertexArray(0);

		glBindVertexArray(VAO);
		glDrawArrays(GL_LINES, 0, 2);
		glBindVertexArray(0);
	}
	virtual void drawContactPoint(const btVector3 &, const btVector3 &, btScalar, int, const btVector3 &) {}
	virtual void reportErrorWarning(const char *) {}
	virtual void draw3dText(const btVector3 &, const char *) {}
	virtual void setDebugMode(int mode) {
		m = mode;
	}
	int getDebugMode(void) const { 
		return 3; 
	}
	int m;
};

#endif