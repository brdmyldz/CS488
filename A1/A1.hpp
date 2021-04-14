// Termm--Fall 2020

#pragma once

#include <glm/glm.hpp>

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"

#include "maze.hpp"
#include "avatar.hpp"

class A1 : public CS488Window {
public:
	A1();
	virtual ~A1();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

private:
	void moveAvatar(int direction);
	void digMaze();
	void reset();
	void initGrid();
	void SphereUploadToVbo();
	void SphereMapToAttributeLocations();
	void uploadVertexDataToVbo();
	void mapVboDataToShaderAttributeLocations();

	Maze maze;
	Avatar avatar;

	// Fields related to the shader and uniforms.
	ShaderProgram m_shader;
	GLint P_uni; // Uniform location for Projection matrix.
	GLint V_uni; // Uniform location for View matrix.
	GLint M_uni; // Uniform location for Model matrix.
	GLint col_uni;   // Uniform location for cube colour.

	// Fields related to grid geometry.
	GLuint m_grid_vao; // Vertex Array Object
	GLuint m_grid_vbo; // Vertex Buffer Object

	// Fields related to cube geometry.
	GLuint m_vao;   // Vertex Array Object
	GLuint m_vbo;   // Vertex Buffer Object

	// Fields related to sphere geometry.
	GLuint sphere_vao;   // Vertex Array Object
	GLuint sphere_vbo;   // Vertex Buffer Object

	//-- Vertex Attribute Index Locations:
	GLuint m_position_attrib_location;
	GLuint m_color_attrib_location;

	// Matrices controlling the camera and projection.
	glm::mat4 proj;
	glm::mat4 view;

	float colour[3];
	int current_col;
	bool persist = false;
	float persistenceRate = 0;
	double lastMousePosX = 0;
	double lastMousePosChecker = 0;
	int curHeight = 0;
	bool isShiftPressed;
  glm::vec3 mazeColor = glm::vec3(0, 0.1, 0.7);
	glm::vec3 floorColor = glm::vec3(0.6, 0.6, 0.6);
};
