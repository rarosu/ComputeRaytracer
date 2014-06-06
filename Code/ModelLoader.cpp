#include "ModelLoader.h"
#include <fstream>
#include <sstream>


void LoadModel( const char* filename, std::vector<Tri>& triangles, AABB& aabb )
{
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uv_coords;

	aabb.m_min = glm::vec4(INT_MAX);
	aabb.m_max = glm::vec4(-INT_MAX);

	std::ifstream file;
	file.open(filename, std::ios_base::in);
	if (file.is_open())	{
		while (!file.eof()) {
			std::string line;
			std::getline(file, line);

			std::stringstream linestream;
			linestream.str(line);
			std::string identifier;
			linestream >> identifier;

			if (identifier == "v") {
				glm::vec3 v;
				linestream >> v.x;
				linestream >> v.y;
				linestream >> v.z;

				aabb.m_min.x = glm::min(aabb.m_min.x, v.x);
				aabb.m_min.y = glm::min(aabb.m_min.y, v.y);
				aabb.m_min.z = glm::min(aabb.m_min.z, v.z);
				aabb.m_max.x = glm::max(aabb.m_max.x, v.x);
				aabb.m_max.y = glm::max(aabb.m_max.y, v.y);
				aabb.m_max.z = glm::max(aabb.m_max.z, v.z);

				vertices.push_back(v);
			} else if (identifier == "vt") {
				glm::vec2 v;
				linestream >> v.x;
				linestream >> v.y;

				v.y = 1.0f - v.y;

				uv_coords.push_back(v);
			} else if (identifier == "f") {
				Tri t;
				
				for (int i = 0; i < 3; ++i)	{
					int index;
					
					// Get corner position
					linestream >> index;
					index--;
					t.m_corners[i] = glm::vec4(vertices[index], 1.0f);
					linestream.ignore();
					
					// Get uv coord
					linestream >> index;
					index--;
					t.m_uv[i] = uv_coords[index];
					linestream.ignore();
					
					// Throw away normal
					linestream >> index;
					linestream.ignore();
				}

				glm::vec3 e1 = glm::vec3(t.m_corners[1]) - glm::vec3(t.m_corners[0]);
				glm::vec3 e2 = glm::vec3(t.m_corners[2]) - glm::vec3(t.m_corners[0]);
				t.m_normal = glm::vec4(glm::normalize(glm::cross(e1, e2)), 0.0f);

				triangles.push_back(t);
			}
		}
	}
}

void LoadMaterial( const char* filename )
{

}
