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

// Mesh vertex format
struct Vtx {
	glm::vec3 pos;		// Position
	glm::vec3 norm;		// Normal
};

// Global state
GLint width, height;
unsigned int viewmode;	// View triangle or obj file
GLuint shader;			// Shader program
GLuint uniXform;		// Shader location of xform mtx
GLuint vao;				// Vertex array object
GLuint vbuf;			// Vertex buffer
GLuint ibuf;
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
vector<glm::u8vec3> textureData;	
GLuint texture;			

u8vec3 bgColor;

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
void initTexture();
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
float ray_triangle_intersect(Ray ray, vector<Vtx> meshVertices);
vector<Vtx> get_coordinates();  

void GLCRender();

int main(int argc, char** argv) {
	try {
		// Initialize
		initState();
		initGLUT(&argc, argv);
		initOpenGL();
		// initTriangle();
		initCamera();
		initTexture();
		initObj();

	} catch (const exception& e) {
		// Handle any errors
		cerr << "Fatal error: " << e.what() << endl;
		cleanup();
		return -1;
	}

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
	ibuf = 0;
	vcount = 0;
	mesh = NULL;
	texture = 0;

	camCoords = vec3(0.0, 0.0, 0.0);
	camRot = false;

	// ADDED BY MASHIAT
	velocity = vec3(0.3, 0.3, 0);
	debug = false;

	viewPlaneStartPos = vec3(-1.0f, -1.0f, 1.0f);

	bgColor = u8vec3(255, 255, 255);

	// Set window and context settings
	width = 500; height = 500;
	textureData.resize(width * height, bgColor);


}

void initGLUT(int* argc, char** argv) {
	
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
	glClearColor(bgColor.x / 255.0f, bgColor.y / 255.0f, bgColor.z / 255.0f, 1.0f);
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
	
	GLuint uniTex = glGetUniformLocation(shader, "tex");

	glUseProgram(shader);
	glUniform1i(uniTex, 0);
	glUseProgram(0);

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
	

	// generateRay( vec3(0.7f, 0.3f, 1) );
	// ray_triangle_intersect(Ray(vec3(0, 0, 0), vec3(1, 0, 0)));
	// vector<Vtx> meshVertices = get_coordinates();
	// float t = ray_triangle_intersect(Ray(vec3(0, 0, 0), vec3(0, 0, 1)), meshVertices);

}

void initTexture()
{
	// Create a surface (quad) to draw the texture onto
	struct vert {
		glm::vec3 pos;	// 2D Position (assume z=0)
		glm::vec2 tc;	// Texture coordinates
	};
	vector<vert> verts = {
		{ glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec2(0.0f, 0.0f) },
		{ glm::vec3( 1.0f, -1.0f, 1.0f), glm::vec2(1.0f, 0.0f) },
		{ glm::vec3( 1.0f,  1.0f, 1.0f), glm::vec2(1.0f, 1.0f) },
		{ glm::vec3(-1.0f,  1.0f, 1.0f), glm::vec2(0.0f, 1.0f) },
	};
	vector<GLubyte> ids = { 0, 1, 2, 2, 3, 0 };
	vcount = ids.size();

	// Create vertex array object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create vertex buffer
	glGenBuffers(1, &vbuf);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vert), verts.data(), GL_STATIC_DRAW);
	// Specify vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vert), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vert), (GLvoid*)sizeof(glm::vec3));
	// Create index buffer
	glGenBuffers(1, &ibuf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ids.size() * sizeof(GLubyte), ids.data(), GL_STATIC_DRAW);

	// Cleanup state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Create texture object
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	assert(glGetError() == GL_NO_ERROR);

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

	stVertices = perspectiveSTVertices;
	cameraMode = PERSPECTIVE;

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

	// changing z of rayOrigin so that it shows everything within z = 0 plane too
	rayOrigin.z = -5.0f; // NOT SURE

	vec3 rayDir = normalize( curPixelPos - uvi );
	if(debug == true){
		Ray temp = Ray(rayOrigin, rayDir);
		temp.printRay();
	}

	return Ray(rayOrigin, rayDir);
	
}

float ray_triangle_intersect(Ray ray, vector<Vtx> meshVertices){

	// Decide whether the ray can interact with the plane
	if ( fabs( dot( meshVertices[0].norm, ray.getDir() ) ) < 0.001) {
		// It means ray and triangle plane will not have intersection
		return -1;
	}
	// ray.printRay();
	// std::cout << meshVertices[0].norm.x << ", " << meshVertices[0].norm.y << ", " << meshVertices[0].norm.z << std::endl;
	// barycentric coordinates calculation

	float ax = meshVertices[0].pos.x;
	float ay = meshVertices[0].pos.y;
	float az = meshVertices[0].pos.z;

	float bx = meshVertices[1].pos.x;
	float by = meshVertices[1].pos.y;
	float bz = meshVertices[1].pos.z;

	float cx = meshVertices[2].pos.x;
	float cy = meshVertices[2].pos.y;
	float cz = meshVertices[2].pos.z;

	float r0x = ray.getOrigin().x;
	float r0y = ray.getOrigin().y;
	float r0z = ray.getOrigin().z;

	float rdx = ray.getDir().x;
	float rdy = ray.getDir().y;
	float rdz = ray.getDir().z;

	mat3 A = mat3(ax - bx, ay - by, az - bz,
				ax - cx, ay - cy, az - cz,
				rdx, rdy, rdz);
	float detA = determinant(A);

	float beta = determinant(mat3(ax - r0x, ay - r0y, az - r0z,
				ax - cx, ay - cy, az - cz,
				rdx, rdy, rdz)) / detA;

	float gamma = determinant(mat3(ax - bx, ay - by, az - bz,
				ax - r0x, ay - r0y, az - r0z,
				rdx, rdy, rdz)) / detA;

	float t = determinant(mat3(ax - bx, ay - by, az - bz,
				ax - cx, ay - cy, az - cz,
				ax - r0x, ay - r0y, az - r0z)) / detA;

	// std::cout << " Ray triangle intersection: " << std::endl;
	// std::cout << "Beta: "<< beta << " , Gamma: " << gamma << " ,t: " << t << std::endl;
		
	if(beta > 0 && gamma > 0 && t > 0 && (beta + gamma) < 1 ){
		return t;
	}
	return -1;

}

vector<Vtx> get_coordinates(){

	vector<Vtx> meshVertices;
	vector<vec3> rawVerts = vector<vec3>(mesh->raw_vertices.size());

	// CALCULATE COORDINATES FOR MESH OBJECTS
	mat4 xform;
	mat4 view = translate(mat4(1.0f), vec3(0.0, 0.0, -camCoords.z)); 
	mat4 rot = rotate(mat4(1.0f), radians(camCoords.y), vec3(1.0, 0.0, 0.0));
	rot = rotate(rot, radians(camCoords.x), vec3(0.0, 1.0, 0.0));
	xform = view * rot;
	if(debug == true ){
		std::cout << "Inside get coordinates func " << std::endl;
		std::cout << "Xform Matrix: " << std::endl;
		for(int i = 0 ; i < 4 ; i++ )
		{
			for(int j = 0 ; j < 4 ; j++ )
			{
				std::cout << xform[i][j] << " ";
			}
			std::cout << std::endl;
		}
	}
	for (int i = 0; i < mesh->raw_vertices.size(); i++) {
		vec4 rawVert = vec4(mesh->raw_vertices[i], 1.f);
		// std::cout << rawVert.x << " " << rawVert.y << " " << rawVert.z << " --> ";
		rawVert = xform * rawVert;
		rawVerts[i] = vec3(rawVert.x, rawVert.y, rawVert.z ); // CHECK FOR THE Z COORDINATES
		// std::cout << rawVerts[i].x << ", " << rawVerts[i].y << ", " << rawVerts[i].z << std::endl;
	}

	// Regenerate the vertices
	meshVertices = vector<Vtx>(mesh->v_elements.size());
	for (int i = 0; i < mesh->v_elements.size(); i += 3) {
		// Store positions
		meshVertices[i + 0].pos = rawVerts[mesh->v_elements[i + 0]];
		meshVertices[i + 1].pos = rawVerts[mesh->v_elements[i + 1]];
		meshVertices[i + 2].pos = rawVerts[mesh->v_elements[i + 2]];
		// Calculate normals
		vec3 normal = normalize(cross(meshVertices[i + 1].pos - meshVertices[i + 0].pos,
			meshVertices[i + 2].pos - meshVertices[i + 0].pos));
		meshVertices[i + 0].norm = normal;
		meshVertices[i + 1].norm = normal;
		meshVertices[i + 2].norm = normal;
	}
	// std::cout << "Mesh Vertices: " << std::endl;
	// for (int i = 0; i < mesh->v_elements.size(); i += 1) {
	// 	// Store positions
	// 	std::cout << "Positions: " << std::endl;
	// 	std::cout << meshVertices[i].pos.x << ", " << meshVertices[i].pos.y << ", " << meshVertices[i].pos.z << std::endl;
	// 	std::cout << "Normal: " << std::endl;
	// 	std::cout << meshVertices[i].norm.x << ", " << meshVertices[i].norm.y << ", " << meshVertices[i].norm.z << std::endl;

	// }
	return meshVertices;

}

void GLCRender(){
	vector<Vtx> meshVertices = get_coordinates();
	std::cout << "Mesh Vertices: " << std::endl;
	for (int i = 0; i < mesh->v_elements.size(); i += 1) {
		// Store positions
		std::cout << "Positions: " << std::endl;
		std::cout << meshVertices[i].pos.x << ", " << meshVertices[i].pos.y << ", " << meshVertices[i].pos.z << std::endl;
		std::cout << "Normal: " << std::endl;
		std::cout << meshVertices[i].norm.x << ", " << meshVertices[i].norm.y << ", " << meshVertices[i].norm.z << std::endl;

	}

	// std::cout << "width, height: " << width << ", " << height << std::endl;
	
	for(int i = 0 ; i < width ; i++ ){
		for(int j = 0 ; j < height ; j++ ){
			// iterate through each pixel and compute their coordinates
			vec3 curPixelPos = computeCurPixelPos(vec2(j, i) );
			if(debug == true )
			{
				std::cout << curPixelPos.x << ", " << curPixelPos.y << ", " << curPixelPos.z << std::endl; 
			}

			// generate Ray with curPixelPos
			Ray genRay = generateRay(curPixelPos);

			// std::cout << "Current Pixel Pos: " << curPixelPos.x << ", " << curPixelPos.y << ", " << curPixelPos.z << std::endl; 
			// genRay.printRay();

			// see if ray intersects with triangle
			float t = ray_triangle_intersect(genRay, meshVertices);
			if(t != - 1){
				// std::cout <<"1 ";
				textureData[ i * height + j ] = u8vec3(255, 0, 0);
			}
			else{
				// std::cout << "0 ";
				textureData[ i * height + j ] = bgColor;
			}

		}
		// std::cout << std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
	glBindTexture(GL_TEXTURE_2D, 0);

}

void display() {
	try {
		// Clear the back buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get ready to draw
		glUseProgram(shader);

		mat4 xform(1.0f);
		float aspect = (float)width / (float)height;
		// Create perspective projection matrix
		// mat4 proj = perspective(45.0f, aspect, 0.1f, 100.0f);
		// // Create view transformation matrix
		// mat4 view = translate(mat4(1.0f), vec3(0.0, 0.0, -camCoords.z)); 
		// mat4 rot = rotate(mat4(1.0f), radians(camCoords.y), vec3(1.0, 0.0, 0.0));
		// rot = rotate(rot, radians(camCoords.x), vec3(0.0, 1.0, 0.0));
		// xform = view * rot;

		switch (viewmode) {
		case VIEWMODE_TRIANGLE:
		// 	glBindVertexArray(vao);
		// 	// Send transformation matrix to shader
		// 	glUniformMatrix4fv(uniXform, 1, GL_FALSE, value_ptr(xform));
		// 	// Draw the triangle
		// 	glDrawArrays(GL_TRIANGLES, 0, vcount);
		// 	glBindVertexArray(0);
			break;

		case VIEWMODE_OBJ: {
			// Load model on demand
			if (!mesh) mesh = new Mesh("models/triangle.obj");

			// // Scale and center mesh using bounding box
			// pair<vec3, vec3> meshBB = mesh->boundingBox();
			// mat4 fixBB = scale(mat4(1.0f), vec3(1.0f / length(meshBB.second - meshBB.first)));
			// fixBB = glm::translate(fixBB, -(meshBB.first + meshBB.second) / 2.0f);
			// // Concatenate all transformations and upload to shader
			// xform = xform * fixBB;
			glUniformMatrix4fv(uniXform, 1, GL_FALSE, value_ptr(xform));
			

			// Draw the mesh
			// mesh->draw();

			
			// GLC computation
			GLCRender();

			
			break;
			}
		}
		assert(glGetError() == GL_NO_ERROR);
		// Draw the textured quad
		glBindVertexArray(vao);
		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glDrawElements(GL_TRIANGLES, vcount, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);

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

	if (ibuf) { glDeleteBuffers(1, &ibuf); ibuf = 0; }
	if (texture) { glDeleteTextures(1, &texture); texture = 0; }

}
