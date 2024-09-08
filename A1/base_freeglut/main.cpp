#include <iostream>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gl_core_3_3.h"
#include <GL/freeglut.h>
#include "util.hpp"
#include "mesh.hpp"
#include "ray.hpp"
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
vec3 velocity;
pair<vec3, vec3> meshBB;
bool debug;

mat3 uvVertices, orthographicSTVertices, perspectiveSTVertices;
mat3 stVertices;
vec3 viewPlaneStartPos;
GLint cameraMode;

// Constants
const int MENU_VIEWMODE = 0;		// Toggle view mode
const int MENU_EXIT = 1;			// Exit application
const int VIEWMODE_TRIANGLE = 0;	// View triangle
const int VIEWMODE_OBJ = 1;			// View obj-loaded mesh
const int PERSPECTIVE = 2;
const int ORTHO = 3;

// Initialization functions
void initState();
void initGLUT(int* argc, char** argv);
void initOpenGL();
void initTriangle();
void initObj();
void initCamera();

// Callback functions
void display();
void reshape(GLint width, GLint height);
void keyRelease(unsigned char key, int x, int y);
void mouseBtn(int button, int state, int x, int y);
void mouseMove(int x, int y);
void idle();
void menu(int cmd);
void cleanup();

vec3 computeCurPixelPos(vec2 screenCoord);
Ray generateRay(vec3 curPixelPos);
GLint ray_triangle_intersect(Ray ray);
void GLCRender();

int main(int argc, char** argv) {
	try {
		// Initialize
		initState();
		initGLUT(&argc, argv);
		initOpenGL();
		// initTriangle();
		initCamera();
		initObj();

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

	camCoords = vec3(0.0, 0.0, 1.0);
	camRot = false;

	// ADDED BY MASHIAT
	velocity = vec3(0.3, 0.3, 0);
	debug = true;

	viewPlaneStartPos = vec3(-1.0f, -1.0f, 1.0f);

}

void initGLUT(int* argc, char** argv) {
	// Set window and context settings
	width = 400; height = 400;
	glutInit(argc, argv);
	glutInitWindowSize(width, height);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	// Create the window
	glutCreateWindow("FreeGlut Window");

	// Create a menu
	glutCreateMenu(menu);
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
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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
		{ vec3(0.433f, -0.25f, 0.0f), vec3(1.0, 0.0, 0.0) },
		{ vec3(0.0f, 0.5f, 0.0f), vec3(1.0, 0.0, 0.0) }
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

void initObj() {
	// mat4 xform;
	// float aspect = (float)width / (float)height;
	// // Create perspective projection matrix
	// mat4 proj = perspective(45.0f, aspect, 0.1f, 100.0f);
	// // Create view transformation matrix
	// mat4 view = translate(mat4(1.0f), vec3(0.0, 0.0, -camCoords.z)); 
	// mat4 rot = rotate(mat4(1.0f), radians(camCoords.y), vec3(1.0, 0.0, 0.0));
	// rot = rotate(rot, radians(camCoords.x), vec3(0.0, 1.0, 0.0));
	// xform = proj * view * rot;

	if (!mesh) mesh = new Mesh("models/triangle.obj");
	// Scale and center mesh using bounding box
	meshBB = mesh->boundingBox();	
	

	generateRay( vec3(0.7f, 0.3f, 1) );
	ray_triangle_intersect(Ray(vec3(0, 0, 0), vec3(1, 0, 0)));
}

void initCamera()
{
	uvVertices = mat3(0.0f, 0.0f , 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f);
	
	perspectiveSTVertices = mat3(0.0f, 0.0f , 1.0f,
								3.0f, 0.0f, 1.0f,
								0.0f, 3.0f, 1.0f);

	orthographicSTVertices = mat3(0.0f, 0.0f , 1.0f,
								1.0f, 0.0f, 1.0f,
								0.0f, 1.0f, 1.0f);

	stVertices = orthographicSTVertices;
	cameraMode = ORTHO;

}

vec3 computeCurPixelPos(vec2 screenCoord)
{
	// suppose the view plane is in the same plane as the uv plane 
	// which is in z = 1 plane 
	// the viewplane has a range of [-1, 1] in both x and y axis in z = 1 plane
	// so if my screenwidth and height is 512x512, then 
	// to calculate the current pixel position, I will start from -1, -1 and count accordingly
	
	vec3 curPixelPos = viewPlaneStartPos + vec3(screenCoord.x * 2.0f / width, screenCoord.y * 2.0f / height, 0 );
	return curPixelPos;
}

Ray generateRay(vec3 curPixelPos){
	// as it is in z = 1 plane, same as st plane
	// we need to find out the alpha and beta for the coordinates of the st plane
	// we can use the same alpha, beta to get the coordinates of the uv plane
	// then we can generate a ray connecting the curPixelPos with the coord of the uv plane

	// using barycentric coordinates, determining alpha, beta from st plane
	// CHECK FOR ROW MAJOR AND COLUMN MAJOR ISSUE
	// --------------------------------------------------------
	mat2 M = mat2(stVertices[0].x - stVertices[2].x, stVertices[0].y - stVertices[2].y,
				stVertices[1].x - stVertices[2].x, stVertices[1].y - stVertices[2].y); 
	vec2 V = vec2(curPixelPos.x - stVertices[2].x, curPixelPos.y - stVertices[2].y);

	vec2 result = inverse(M) * V;
	float alpha = result.x;
	float beta = result.y;

	if(debug == true ){
		std::cout << "alpha: " << alpha << " , beta: " << beta << endl;
	}

	// compute the coordinate for the UV planeperspective
	vec3 uvi =  alpha * uvVertices[0] + beta * uvVertices[1] + ( 1 - alpha - beta) * uvVertices[2];
	if( debug == true ){
		std::cout << "STI: " << uvi.x << ", " << uvi.y << ", " << uvi.z << std::endl;
	}

	// calculate ray center and direction
	// ray will be basically uvi - curPixelPos(on uv plane)
	vec3 rayOrigin = uvi;
	vec3 rayDir = normalize( curPixelPos - uvi );
	if(debug == true){
		Ray temp = Ray(rayOrigin, rayDir);
		temp.printRay();
	}

	return Ray(rayOrigin, rayDir);
	
}

GLint ray_triangle_intersect(Ray ray){

	
	return 0;

}

void GLCRender(){
	for(int i = 0 ; i < width; i++ ){
		for(int j = 0 ; j < height ; j++ ){
			// iterate through each pixel and compute their coordinates
			vec3 curPixelPos = computeCurPixelPos(vec2(i, j) );
			// if(debug == true )
			// {
			// 	std::cout << curPixelPos.x << ", " << curPixelPos.y << ", " << curPixelPos.z << std::endl; 
			// }

			// generate Ray with curPixelPos
			Ray genRay = generateRay(curPixelPos);

			// see if ray intersects with triangle

		}
	}

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

			
			// GLC computation
			// GLCRender();
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
	// // add velocity to the bounding box coordinates of the object
	// meshBB.first += velocity;
	// meshBB.second += velocity;



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
