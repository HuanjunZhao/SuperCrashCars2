#pragma once

#include <vector>

#include "Geometry.h"
#include "ShaderProgram.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

struct Process { CPU_Geometry cpuGeom; GPU_Geometry gpuGeom; };

class GLMesh {

public:
	GLMesh(ShaderProgram& shader);
	~GLMesh();

	void createPlane(int size, glm::vec3 color);
	void createCube(float scale, glm::vec3 color);
	void createSphere(float radius, int numSectors, glm::vec3 color);

	void render(glm::mat4& TM);
	void render();

private:
	Process m_process;
	ShaderProgram& m_shader;
	glm::mat4 m_TM;

	std::vector<glm::vec3> generateQuads(std::vector<std::vector<glm::vec3>>& points);

};