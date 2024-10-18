#include <iostream>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gl_core_3_3.h"
#include <GL/freeglut.h>
#include "util.hpp"
#include "mesh.hpp"
using namespace std;
using namespace glm;

// Global state
GLint width, height;
unsigned int viewmode;	// View triangle or obj file
GLuint shader;			// Shader program
GLuint uniXform;		// Shader location of xform mtx
GLuint vao;				// Vertex array object
GLuint vbuf;			// Vertex buffer
GLsizei vcount;			// Number of vertices
Mesh* mesh;				// Mesh loaded from .obj file


// Camera state
vec3 camCoords;			// Spherical coordinates (theta, phi, radius) of the camera
bool camRot;			// Whether the camera is currently rotating
vec2 camOrigin;			// Original camera coordinates upon clicking
vec2 mouseOrigin;		// Original mouse coordinates upon clicking

// ADDED BY MASHIAT
bool debug;

// Constants
const int MENU_VIEWMODE = 0;		// Toggle view mode
const int MENU_EXIT = 1;			// Exit application
const int VIEWMODE_TRIANGLE = 0;	// View triangle
const int VIEWMODE_OBJ = 1;			// View obj-loaded mesh

// Initialization functions
void initState();
void initGLUT(int* argc, char** argv);
void initOpenGL();
void initTriangle();

// Callback functions
void display();
void reshape(GLint width, GLint height);
void keyRelease(unsigned char key, int x, int y);
void mouseBtn(int button, int state, int x, int y);
void mouseMove(int x, int y);
void idle();
void menu(int cmd);
void cleanup();

int main(int argc, char** argv) {
	try {
		// Initialize
		initState();
		initGLUT(&argc, argv);
		initOpenGL();
		initTriangle();

	} catch (const exception& e) {
		// Handle any errors
		cerr << "Fatal error: " << e.what() << endl;
		cleanup();
		return -1;
	}

	// Execute main loop
	glutMainLoop();

	return 0;
}

void initState() {
	// Initialize global state
	width = 0;
	height = 0;
	viewmode = VIEWMODE_OBJ;
	shader = 0;
	uniXform = 0;
	vao = 0;
	vbuf = 0;
	vcount = 0;
	mesh = NULL;

	camCoords = vec3(0.0, 0.0, 10.0);
	camRot = false;

	// ADDED BY MASHIAT
	debug = false;

}

void initGLUT(int* argc, char** argv) {
	// Set window and context settings
	width = 800; height = 600;
	glutInit(argc, argv);
	glutInitWindowSize(width, height);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	// Create the window
	glutCreateWindow("FreeGlut Window");

	// Create a menu
	GLuint submenu =  glutCreateMenu(menu);
	glutAddMenuEntry("Toggle view mode", MENU_VIEWMODE);
	glutAddMenuEntry("Exit", MENU_EXIT);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// GLUT callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardUpFunc(keyRelease);
	glutMouseFunc(mouseBtn);
	glutMotionFunc(mouseMove);
	glutIdleFunc(idle);
	glutCloseFunc(cleanup);
}

void initOpenGL() {
	// Set clear color and depth
	glClearColor(0.0f, 0.f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	// Compile and link shader program
	vector<GLuint> shaders;
	shaders.push_back(compileShader(GL_VERTEX_SHADER, "sh_v.glsl"));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "sh_f.glsl"));
	shader = linkProgram(shaders);
	// Release shader sources
	for (auto s = shaders.begin(); s != shaders.end(); ++s)
		glDeleteShader(*s);
	shaders.clear();
	// Locate uniforms
	uniXform = glGetUniformLocation(shader, "xform");
	assert(glGetError() == GL_NO_ERROR);
}

void initTriangle() {
	// Create a colored triangle
	struct vert {
		vec3 pos;	// Vertex position
		vec3 norm;	// Vertex normal
	};
	vector<vert> verts = {
		{ vec3(-0.433f, -0.25f, 0.0f), vec3(1.0, 0.0, 0.0) },
		{ vec3(0.433f, -0.25f, 0.0f), vec3(0.0, 1.0, 0.0) },
		{ vec3(0.0f, 0.5f, 0.0f), vec3(0.0, 0.0, 1.0) }
	};
	vcount = verts.size();


	// Create vertex array object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create vertex buffer
	glGenBuffers(1, &vbuf);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBufferData(GL_ARRAY_BUFFER, vcount * sizeof(vert), verts.data(), GL_STATIC_DRAW);
	// Specify vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vert), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vert), (GLvoid*)sizeof(vec3));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	assert(glGetError() == GL_NO_ERROR);

}



void display() {
	try {
		// Clear the back buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get ready to draw
		glUseProgram(shader);

		mat4 xform;
		float aspect = (float)width / (float)height;
		// Create perspective projection matrix
		mat4 proj = perspective(45.0f, aspect, 0.1f, 100.0f);
		// Create view transformation matrix
		mat4 view = translate(mat4(1.0f), vec3(0.0, 0.0, -camCoords.z)); 
		mat4 rot = rotate(mat4(1.0f), radians(camCoords.y), vec3(1.0, 0.0, 0.0));
		rot = rotate(rot, radians(camCoords.x), vec3(0.0, 1.0, 0.0));
		xform = proj * view * rot;

		switch (viewmode) {
		case VIEWMODE_TRIANGLE:
			glBindVertexArray(vao);
			// Send transformation matrix to shader
			glUniformMatrix4fv(uniXform, 1, GL_FALSE, value_ptr(xform));
			// Draw the triangle
			glDrawArrays(GL_TRIANGLES, 0, vcount);
			glBindVertexArray(0);
			break;

		case VIEWMODE_OBJ: {
			// Load model on demand
			if (!mesh) mesh = new Mesh("models/cube.obj");

			// Scale and center mesh using bounding box
			pair<vec3, vec3> meshBB = mesh->boundingBox();
			mat4 fixBB = scale(mat4(1.0f), vec3(1.0f / length(meshBB.second - meshBB.first)));
			fixBB = glm::translate(fixBB, -(meshBB.first + meshBB.second) / 2.0f);
			// Concatenate all transformations and upload to shader
			xform = xform * fixBB;
			glUniformMatrix4fv(uniXform, 1, GL_FALSE, value_ptr(xform));
			
			// Draw the mesh
			mesh->draw();
			break; }
		}
		assert(glGetError() == GL_NO_ERROR);

		// Revert context state
		glUseProgram(0);

		// Display the back buffer
		glutSwapBuffers();

		glutPostRedisplay();


	} catch (const exception& e) {
		cerr << "Fatal error: " << e.what() << endl;
		glutLeaveMainLoop();
	}
}

void reshape(GLint width, GLint height) {
	::width = width;
	::height = height;
	glViewport(0, 0, width, height);
}

void keyRelease(unsigned char key, int x, int y) {
	switch (key) {
	case 27:	// Escape key
		menu(MENU_EXIT);
		break;
	}
}

void mouseBtn(int button, int state, int x, int y) {
	if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
		// Activate rotation mode
		camRot = true;
		camOrigin = vec2(camCoords);
		mouseOrigin = vec2(x, y);
	}
	if (state == GLUT_UP && button == GLUT_LEFT_BUTTON) {
		// Deactivate rotation
		camRot = false;
	}
	if (button == 3) {
		camCoords.z = clamp(camCoords.z - 0.1f, 0.1f, 10.0f);
		glutPostRedisplay();
	}
	if (button == 4) {
		camCoords.z = clamp(camCoords.z + 0.1f, 0.1f, 10.0f);
		glutPostRedisplay();
	}
}

void mouseMove(int x, int y) {
	if (camRot) {
		// Convert mouse delta into degrees, add to rotation
		float rotScale = glm::min(width / 450.0f, height / 270.0f);
		vec2 mouseDelta = vec2(x, y) - mouseOrigin;
		vec2 newAngle = camOrigin + mouseDelta / rotScale;
		newAngle.y = clamp(newAngle.y, -90.0f, 90.0f);
		while (newAngle.x > 180.0f) newAngle.x -= 360.0f;
		while (newAngle.y < -180.0f) newAngle.y += 360.0f;
		if (length(newAngle - vec2(camCoords)) > FLT_EPSILON) {
			camCoords.x = newAngle.x;
			camCoords.y = newAngle.y;
			glutPostRedisplay();
		}
	}
}

void idle() {
}

void menu(int cmd) {
	switch (cmd) {
	case MENU_VIEWMODE:
		viewmode = (viewmode + 1) % 2;
		glutPostRedisplay();	// Tell GLUT to redraw the screen
		break;

	case MENU_EXIT:
		glutLeaveMainLoop();
		break;
	}
}

void cleanup() {
	// Release all resources
	if (shader) { glDeleteProgram(shader); shader = 0; }
	uniXform = 0;
	if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
	if (vbuf) { glDeleteBuffers(1, &vbuf); vbuf = 0; }
	vcount = 0;
	if (mesh) { delete mesh; mesh = NULL; }
}
