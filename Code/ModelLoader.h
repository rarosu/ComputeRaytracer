#pragma once

#include "glm/glm.hpp"
#include <vector>

struct Tri {
	glm::vec4 m_normal;
	glm::vec4 m_corners[3];
	glm::vec2 m_uv[3];
	unsigned int m_materialIndex;
};

struct AABB {
	glm::vec4 m_min;
	glm::vec4 m_max;
};


void LoadModel(const char* filename, std::vector<Tri>& triangles, AABB& aabb);
void LoadMaterial(const char* filename);
