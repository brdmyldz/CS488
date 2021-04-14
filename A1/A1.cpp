// Termm--Fall 2020

#include "A1.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>
#include <vector>

#include <sys/types.h>
#include <unistd.h>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
using namespace std;

#define M_PI          3.14159265358979323846
static const size_t DIM = 16;

//----------------------------------------------------------------------------------------
// Constructor
A1::A1()
	: current_col( 0 ), maze(DIM)
{
	colour[0] = 0.0f;
	colour[1] = 0.0f;
	colour[2] = 0.0f;
}

//----------------------------------------------------------------------------------------
// Destructor
A1::~A1()
{}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A1::init()
{
	// Initialize random number generator
	int rseed=getpid();
	srandom(rseed);
	// Print random number seed in case we want to rerun with
	// same random numbers
	cout << "Random number seed = " << rseed << endl;
	maze.digMaze();


	/*maze.printMaze();
	for (int i = 0; i < DIM; ++i){
		cout << maze.getValue(0, i);
	}
	cout << endl;*/

	// Set the background colour.
	glClearColor( 0.3, 0.5, 0.7, 1.0 );

	// Build the shader
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(
		getAssetFilePath( "VertexShader.vs" ).c_str() );
	m_shader.attachFragmentShader(
		getAssetFilePath( "FragmentShader.fs" ).c_str() );
	m_shader.link();

	// Set up the uniforms
	P_uni = m_shader.getUniformLocation( "P" );
	V_uni = m_shader.getUniformLocation( "V" );
	M_uni = m_shader.getUniformLocation( "M" );
	col_uni = m_shader.getUniformLocation( "colour" );

	initGrid();
	SphereUploadToVbo();
	SphereMapToAttributeLocations();
	uploadVertexDataToVbo();
	mapVboDataToShaderAttributeLocations();

	// Set up initial view and projection matrices (need to do this here,
	// since it depends on the GLFW window being set up correctly).
	view = glm::lookAt(
		glm::vec3( 0.0f, 2.*float(DIM)*2.0*M_SQRT1_2, float(DIM)*2.0*M_SQRT1_2 ),
		glm::vec3( 0.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 1.0f, 0.0f ) );

	proj = glm::perspective(
		glm::radians( 30.0f ),
		float( m_framebufferWidth ) / float( m_framebufferHeight ),
		1.0f, 1000.0f );
}

void A1::initGrid()
{
	size_t sz = 3 * 2 * 2 * (DIM+3);

	float *verts = new float[ sz ];
	size_t ct = 0;
	for( int idx = 0; idx < DIM+3; ++idx ) {
		verts[ ct ] = -1;
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = idx-1;
		verts[ ct+3 ] = DIM+1;
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = idx-1;
		ct += 6;

		verts[ ct ] = idx-1;
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = -1;
		verts[ ct+3 ] = idx-1;
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = DIM+1;
		ct += 6;
	}

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays( 1, &m_grid_vao );
	glBindVertexArray( m_grid_vao );

	// Create the cube vertex buffer
	glGenBuffers( 1, &m_grid_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_grid_vbo );
	glBufferData( GL_ARRAY_BUFFER, sz*sizeof(float),
		verts, GL_STATIC_DRAW );

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation( "position" );
	glEnableVertexAttribArray( posAttrib );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr );

	// Reset state to prevent rogue code from messing with *my*
	// stuff!
	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	// OpenGL has the buffer now, there's no need for us to keep a copy.
	delete [] verts;

	CHECK_GL_ERRORS;
}

// Sphere logic
void A1::SphereUploadToVbo(){
	float radius = 0.5f;
	float sectorCount = 28;
	float stackCount = 11;

	float vertices[1044] = {};  // vertex positions
	vec3 sphereTriangles[1680] = {};

	float x, y, z, xy;
	int k1, k2;

	float sectorStep = 2 * M_PI / sectorCount;
	float stackStep = M_PI / stackCount;
	float sectorAngle, stackAngle;

	int arrayCount = 0;
	for(int i = 0; i <= stackCount; ++i){
    stackAngle = M_PI / 2 - i * stackStep;
    xy = radius * cosf(stackAngle);
    z = radius * sinf(stackAngle);

		for(int j = 0; j <= sectorCount; ++j){
			sectorAngle = j * sectorStep;
			x = xy * cosf(sectorAngle);
			y = xy * sinf(sectorAngle);
		  vertices[arrayCount] = x;
			vertices[arrayCount + 1] = y;
			vertices[arrayCount + 2] = z;
			arrayCount += 3;
		}
	}

	arrayCount = 0;
	for(int i = 0; i < stackCount; ++i){
    k1 = i * (sectorCount + 1);
    k2 = k1 + sectorCount + 1;
		for(int j = 0; j < sectorCount; ++j, ++k1, ++k2){
			int adjK1 = k1 * 3;
			int adjK2 = k2 * 3;
			int adjK1P = (k1 + 1) * 3;
			int adjK2P = (k2 + 1) * 3;

      // k1 => k2 => k1+1
      if(i != 0){
				sphereTriangles[arrayCount] = vec3(vertices[adjK1],
																					 vertices[adjK1 + 1],
																				 	 vertices[adjK1 + 2]);
			 	sphereTriangles[arrayCount + 1] = vec3(vertices[adjK2],
																							 vertices[adjK2 + 1],
																						 	 vertices[adjK2 + 2]);
				sphereTriangles[arrayCount + 2] = vec3(vertices[adjK1P],
																							 vertices[adjK1P + 1],
																						 	 vertices[adjK1P + 2]);
				arrayCount += 3;
      }
      // k1+1 => k2 => k2+1
      if(i != (stackCount-1)){
				sphereTriangles[arrayCount] = vec3(vertices[adjK1P],
																					 vertices[adjK1P + 1],
																					 vertices[adjK1P + 2]);
				sphereTriangles[arrayCount + 1] = vec3(vertices[adjK2],
																					 vertices[adjK2 + 1],
																				 	 vertices[adjK2 + 2]);
				sphereTriangles[arrayCount + 2] = vec3(vertices[adjK2P],
																					 vertices[adjK2P + 1],
																					 vertices[adjK2P + 2]);
				arrayCount += 3;
      }
		}
	}

	// Generate a vertex buffer object to hold the triangle's vertex data.
	glGenBuffers(1, &sphere_vbo);

	//-- Upload triangle vertex data to the vertex buffer:
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
	glBufferData(GL_ARRAY_BUFFER,
									sizeof(sphereTriangles),
									sphereTriangles,
									GL_STATIC_DRAW
		 );


	// Unbind the target GL_ARRAY_BUFFER, now that we are finished using it.
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	CHECK_GL_ERRORS;

}

void A1::SphereMapToAttributeLocations()
{
 // Generate and bind the VAO that will store the data mapping.
 glGenVertexArrays(1, &sphere_vao);
 glBindVertexArray(sphere_vao);

 // Map vertex positions from m_vbo into vertex attribute slot
 {
	 glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
				GLsizei stride = sizeof(vec3);
				const GLvoid * offset = 0;
	 glVertexAttribPointer(m_position_attrib_location, 3, GL_FLOAT, GL_FALSE, stride,
															reinterpret_cast<const GLvoid *>(offset));

	 // Enable attribute location for rendering
	 glEnableVertexAttribArray(m_position_attrib_location);
 }

 //-- Unbind target, and restore default values:
 glBindBuffer(GL_ARRAY_BUFFER, 0);
 glBindVertexArray(0);

 CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Cube logic
 */
 void A1::uploadVertexDataToVbo()
 {
 	// Vertex positions for triangle vertices.
 	vec3 triangleVertices[] = {
		// Bottom Triangle 1
 		vec3(0.0f, 0.0f, 0.0f),
 		vec3(0.0f, 0.0f, 1.0f),
 		vec3(1.0f,  0.0f, 0.0f),
		// Bottom Triangle 2
		vec3(1.0f, 0.0f, 1.0f),
		vec3(0.0f, 0.0f, 1.0f),
		vec3(1.0f,  0.0f, 0.0f),
		// Middle Left Triangle 1
		vec3(0.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 1.0f),
		vec3(0.0f,  1.0f, 0.0f),
		// Middle Left Triangle 2
		vec3(0.0f, 0.0f, 0.0f),
		vec3(0.0f, 0.0f, 1.0f),
		vec3(0.0f,  1.0f, 1.0f),
		// Middle Right Triangle 1
		vec3(1.0f, 0.0f, 0.0f),
		vec3(1.0f, 1.0f, 1.0f),
		vec3(1.0f,  1.0f, 0.0f),
		// Middle Right Triangle 2
		vec3(1.0f, 0.0f, 0.0f),
		vec3(1.0f, 0.0f, 1.0f),
		vec3(1.0f,  1.0f, 1.0f),
		// Middle Back Triangle 1
		vec3(0.0f, 0.0f, 0.0f),
		vec3(1.0f, 1.0f, 0.0f),
		vec3(0.0f,  1.0f, 0.0f),
		// Middle Back Triangle 2
		vec3(0.0f, 0.0f, 0.0f),
		vec3(1.0f, 0.0f, 0.0f),
		vec3(1.0f,  1.0f, 0.0f),
		// Middle Front Triangle 1
		vec3(0.0f, 0.0f, 1.0f),
		vec3(1.0f, 1.0f, 1.0f),
		vec3(0.0f,  1.0f, 1.0f),
		// Middle Front Triangle 2
		vec3(0.0f, 0.0f, 1.0f),
		vec3(1.0f, 0.0f, 1.0f),
		vec3(1.0f,  1.0f, 1.0f),
		// Top Triangle 1
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 1.0f, 1.0f),
		vec3(1.0f,  1.0f, 0.0f),
		// Top Triangle 2
		vec3(1.0f, 1.0f, 1.0f),
		vec3(0.0f, 1.0f, 1.0f),
		vec3(1.0f,  1.0f, 0.0f)
};

 	// Generate a vertex buffer object to hold the triangle's vertex data.
 	glGenBuffers(1, &m_vbo);

 	//-- Upload triangle vertex data to the vertex buffer:
 	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
 	glBufferData(GL_ARRAY_BUFFER,
                  sizeof(triangleVertices),
                  triangleVertices,
                  GL_STATIC_DRAW
     );


 	// Unbind the target GL_ARRAY_BUFFER, now that we are finished using it.
 	glBindBuffer(GL_ARRAY_BUFFER, 0);

 	CHECK_GL_ERRORS;
 }

 //----------------------------------------------------------------------------------------
 // Map data from VBOs into vertex shader attribute locations.
 void A1::mapVboDataToShaderAttributeLocations()
 {
 	// Generate and bind the VAO that will store the data mapping.
 	glGenVertexArrays(1, &m_vao);
 	glBindVertexArray(m_vao);

 	// Map vertex positions from m_vbo into vertex attribute slot
 	{
 		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
         GLsizei stride = sizeof(vec3);
         const GLvoid * offset = 0;
 		glVertexAttribPointer(m_position_attrib_location, 3, GL_FLOAT, GL_FALSE, stride,
                               reinterpret_cast<const GLvoid *>(offset));

 		// Enable attribute location for rendering
 		glEnableVertexAttribArray(m_position_attrib_location);
 	}

 	//-- Unbind target, and restore default values:
 	glBindBuffer(GL_ARRAY_BUFFER, 0);
 	glBindVertexArray(0);

 	CHECK_GL_ERRORS;
 }


void A1::moveAvatar(int direction){
	bool isOut = true;
	int newX = avatar.getPositionX();
	int newY = avatar.getPositionY();
	// First check if out of bounds
	switch(direction){
		case 1:
			if(avatar.getPositionX() == -1){
				isOut = false;
			}
			newX -= 1;
			break;
		case 2:
			if(avatar.getPositionX() == 16){
				isOut = false;
			}
			newX += 1;
			break;
		case 3:
			if(avatar.getPositionY() == -1){
				isOut = false;
			}
			newY -= 1;
			break;
		case 4:
			if(avatar.getPositionY() == 16){
				isOut = false;
			}
			newY += 1;
			break;
	}

	// Now check if there is a wall in front of us
	if(isOut){
		// Check if there is a wall
		if(!(maze.getValue(newY, newX)) ||
				 newY < 0 || newY > 15 || newX < 0 || newX > 15){// No wall
			avatar.setPositionX(newX);
			avatar.setPositionY(newY);
		} else { // There is a wall
			if(isShiftPressed){
				maze.setValue(newY, newX, 0);
				avatar.setPositionX(newX);
				avatar.setPositionY(newY);
			}
		}
	}
}

void A1::reset(){
	persistenceRate = 0;
	view = glm::lookAt(
		glm::vec3( 0.0f, 2.*float(DIM)*2.0*M_SQRT1_2, float(DIM)*2.0*M_SQRT1_2 ),
		glm::vec3( 0.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 1.0f, 0.0f ) );

	mazeColor = vec3(0, 0.1, 0.7);
	floorColor = vec3(0.6, 0.6, 0.6);
	current_col = 0;
	colour[0] = 0.0f;
	colour[1] = 0.0f;
	colour[2] = 0.0f;


	avatar.setPositionX(-1);
	avatar.setPositionY(-1);
	avatar.setColor(vec3(1, 0, 0));

	curHeight = 0;
}

void A1::digMaze(){
	if(curHeight == 0){
		curHeight = 1;
	} else {
		maze.digMaze();
	}

	// Find the start cell
	float cell;
	for (int i = 0; i < DIM; ++i){
		if(maze.getValue(0, i) == 0){
			cell = (float) i;
			break;
		}
	}
	avatar.setPositionX(cell);
	avatar.setPositionY(0);
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A1::appLogic()
{
	if(!persist && (lastMousePosX == lastMousePosChecker)){ // Mouse is not moving
		persistenceRate = 0;
	}
	lastMousePosChecker = lastMousePosX;

	view = rotate(view, persistenceRate, vec3(0, 1, 0));
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A1::guiLogic()
{
	// We already know there's only going to be one window, so for
	// simplicity we'll store button states in static local variables.
	// If there was ever a possibility of having multiple instances of
	// A1 running simultaneously, this would break; you'd want to make
	// this into instance fields of A1.
	static bool showTestWindow(false);
	static bool showDebugWindow(true);

	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Debug Window", &showDebugWindow, ImVec2(100,100), opacity, windowFlags);
		if( ImGui::Button( "Quit Application" ) ) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
		}
		if(ImGui::Button("Reset")){
			reset();
		}
		if(ImGui::Button("Dig")){
			digMaze();
		}

		// Eventually you'll create multiple colour widgets with
		// radio buttons.  If you use PushID/PopID to give them all
		// unique IDs, then ImGui will be able to keep them separate.
		// This is unnecessary with a single colour selector and
		// radio button, but I'm leaving it in as an example.

		// Prefixing a widget name with "##" keeps it from being
		// displayed.
		ImGui::PushID( 0 );
		ImGui::ColorEdit3( "##Colour", colour );

		ImGui::RadioButton( "Wall", &current_col, 0 );
		ImGui::SameLine();
		ImGui::RadioButton( "Avatar", &current_col, 1 );
		ImGui::SameLine();
		ImGui::RadioButton( "Floor", &current_col, 2 );
		ImGui::PopID();

		if(current_col == 0){
			mazeColor = vec3(colour[0], colour[1], colour[2]);
		}
		if(current_col == 1){
			avatar.setColor(vec3(colour[0], colour[1], colour[2]));
		}
		if(current_col == 2){
			floorColor = vec3(colour[0], colour[1], colour[2]);
		}

		/*// For convenience, you can uncomment this to show ImGui's massive
		// demonstration window right in your application.  Very handy for
		// browsing around to get the widget you want.  Then look in
		// shared/imgui/imgui_demo.cpp to see how it's done.
		if( ImGui::Button( "Test Window" ) ) {
			showTestWindow = !showTestWindow;
		}*/


		ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );

	ImGui::End();

	if( showTestWindow ) {
		ImGui::ShowTestWindow( &showTestWindow );
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A1::draw()
{
	// Create a global transformation for the model (centre it).
	mat4 W;

	GLsizei numVertices = 36;
	GLsizei numFloorVertices = 6;
	GLsizei numSphereVertices = 1680;

	W = glm::translate( W, vec3( -float(DIM)/2.0f, 0, -float(DIM)/2.0f ) );

	m_shader.enable();
		glEnable( GL_DEPTH_TEST );

		glUniformMatrix4fv( P_uni, 1, GL_FALSE, value_ptr( proj ) );
		glUniformMatrix4fv( V_uni, 1, GL_FALSE, value_ptr( view ) );
		glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );

		// Just draw the grid for now.
		glBindVertexArray( m_grid_vao );
		glUniform3f( col_uni, 1, 1, 1);
		glDrawArrays( GL_LINES, 0, (3+DIM)*4 );

		// Save state
		mat4 origin = W;

		// Set avatar and it's color
		glUniform3fv(col_uni, 1, value_ptr(avatar.getColor()));
		W = translate(W, vec3(avatar.getPositionX() + 0.5, 0.5,
													avatar.getPositionY() + 0.5));
		glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );
		glBindVertexArray(sphere_vao);
		glDrawArrays(GL_TRIANGLES, 0, numSphereVertices);

		// Go back to the original state
		W = origin;
		glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );

		//Set maze cube color
		glUniform3fv(col_uni, 1, value_ptr(mazeColor));

		// Draw the maze
		for (int loopHeight = 0; loopHeight < curHeight; ++loopHeight){ //change this later
			for (int x = 0; x < DIM; ++x){
				for (int y = 0; y < DIM; ++y){
					if(maze.getValue(x ,y) == 1){
						glBindVertexArray(m_vao);
						glDrawArrays(GL_TRIANGLES, 0, numVertices);
					}
					//Transform the cube
					W = translate(W, vec3(1, 0, 0));
					glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );
				}
				// Reset for the next row
				W = translate(W, vec3(-16, 0, 1));
				glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );
			}
			W = translate(W, vec3(0, 1, -16));
			glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );
		}

		// Go back to the original state
		W = origin;
		glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );

		// Set floor polygon color
		glUniform3fv(col_uni, 1, value_ptr(floorColor));

		// Set the floor using cube attribute array
		W = translate(W, vec3(0, 0.1, 0));
		//W = translate(W, vec3(0, 0.1, 0));
		W = scale(W, vec3(DIM));
		glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLES, 0, numFloorVertices);

		// Highlight the active square.
	m_shader.disable();

	// Restore defaults
	glBindVertexArray( 0 );

	CHECK_GL_ERRORS;
}
//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A1::cleanup()
{}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A1::cursorEnterWindowEvent (
		int entered
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool A1::mouseMoveEvent(double xPos, double yPos)
{
	bool eventHandled(false);


	if (!ImGui::IsMouseHoveringAnyWindow()) {

		if(ImGui::IsMouseDragging(0)){ // 0 Stands for Left Mouse Button
			float mouseInterval = (xPos - lastMousePosX) / 100; //100 seems to work fine
			persistenceRate = mouseInterval / 2; // Make it smoother
			view = rotate(view, mouseInterval, vec3(0, 1, 0));
		}
		lastMousePosX = xPos;
	  eventHandled = true;
	}
	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A1::mouseButtonInputEvent(int button, int actions, int mods) {
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		// Handle Mouse Left click and change bool variable
		if(button == GLFW_MOUSE_BUTTON_LEFT){
			if(actions == GLFW_PRESS){
					persist = false;

					eventHandled = true;
			}
			if(actions == GLFW_RELEASE){
					if(persistenceRate != 0){
						persist = true;
					}

					eventHandled = true;
			}
		}
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A1::mouseScrollEvent(double xOffSet, double yOffSet) {
	bool eventHandled(false);
	float varIn = 1.5f; //testing if this is good value
	float varOut = 1/varIn;

	if (yOffSet > 0){
		view = scale(view, vec3(varIn, varIn, varIn));
		eventHandled = true;
	} else {
		view = scale (view, vec3(varOut, varOut, varOut));
		eventHandled = true;
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool A1::windowResizeEvent(int width, int height) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A1::keyInputEvent(int key, int action, int mods) {

	// Fill in with event handling code...
	if( action == GLFW_PRESS) {
		switch(key){
			case GLFW_KEY_SPACE:
				++curHeight;
				return true;
			case GLFW_KEY_BACKSPACE:
				if(curHeight > 0){
					--curHeight;
				}
				return true;
			case GLFW_KEY_LEFT_SHIFT:
				isShiftPressed = true;
				return true;
			case GLFW_KEY_LEFT:
				moveAvatar(1);
				return true;
			case GLFW_KEY_RIGHT:
				moveAvatar(2);
				return true;
			case GLFW_KEY_UP:
				moveAvatar(3);
				return true;
			case GLFW_KEY_DOWN:
				moveAvatar(4);
				return true;
			case GLFW_KEY_Q:
				glfwSetWindowShouldClose(m_window, GL_TRUE);
				return true;
			case GLFW_KEY_D:
				digMaze();
				return true;
			case GLFW_KEY_R:
				reset();
				return true;
		}
	}

	if(action == GLFW_RELEASE){
		switch(key){
			// Assumed Left Shift Key would be enough
			case GLFW_KEY_LEFT_SHIFT:
				isShiftPressed = false;
		}
	}

	return false;
}
