#ifndef OBJLOADER_H
#define OBJLOADER_H

bool loadModel(const char* modelpath, std::vector<unsigned short> &indices, std::vector<glm::vec3> &vertices, std::vector<glm::vec2> &uvs, std::vector<glm::vec3> &normals);

#endif