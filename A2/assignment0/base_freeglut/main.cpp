#include <iostream>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gl_core_3_3.h"
#include <GL/freeglut.h>
#include <cmath>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"
#include "util.hpp"
// #include "mesh.hpp"
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
// Mesh* mesh;				// Mesh loaded from .obj file
GLint uRenderDiscLoc;
GLint uSigma;
GLint alpha;
vec2 imgRange;


// Camera state
vec3 camCoords;			// Spherical coordinates (theta, phi, radius) of the camera
bool camRot;			// Whether the camera is currently rotating
vec2 camOrigin;			// Original camera coordinates upon clicking
vec2 mouseOrigin;		// Original mouse coordinates upon clicking

// ADDED BY MASHIAT
const int gridSize = 64;
vector<GLfloat> points;
vector<GLfloat> colors;
int pointCount;
int imageWidth, imageHeight;
int channels;
bool debug;
float fovAngle = 45.0f;
float pointsZ = 0.0f;

// Constants
const int MENU_VIEWMODE = 0;		// Toggle view mode
const int MENU_EXIT = 1;			// Exit application
const int VIEWMODE_SQUARE = 0;	
const int VIEWMODE_DISC = 1;			
const int VIEWMODE_GAUSS = 2;

// Initialization functions
void initState();
void initGLUT(int* argc, char** argv);
void initOpenGL();
void initTriangle();
void initPoints();
unsigned char* initImage();

// Callback functions
void display();
void reshape(GLint width, GLint height);
void keyRelease(unsigned char key, int x, int y);
void keyPressed(int key, int x, int y);
void mouseBtn(int button, int state, int x, int y);
void mouseMove(int x, int y);
void idle();
void menu(int cmd);
void cleanup();

void squash(vec2 P);



int main(int argc, char** argv) {
	try {
		// Initialize
		initState();
		initGLUT(&argc, argv);
		initOpenGL();
		// initTriangle();
		initPoints();
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
	viewmode = VIEWMODE_SQUARE;
	shader = 0;
	uniXform = 0;
	vao = 0;
	vbuf = 0;
	vcount = 0;
	uRenderDiscLoc = 0;
	uSigma = 0;
	alpha = 0.1;
	imgRange = vec2(32.0f, 32.0f);

	camCoords = vec3(0.0, 0.0, 70.0);
	camRot = false;

	// ADDED BY MASHIAT
	debug = false;
	pointCount = 0;

}

void initGLUT(int* argc, char** argv) {
	// Set window and context settings
	width = 640; 
	height = width;
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
	glutSpecialFunc(keyPressed);

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
	uRenderDiscLoc = glGetUniformLocation(shader, "uRenderDisc");

	uSigma = glGetUniformLocation(shader, "sigma");
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
vec2 determineGridSize()
{
	std::cout << "image width and height: " << imageWidth << ", " << imageHeight << std::endl;
	if(imageWidth == imageHeight) return vec2(gridSize, gridSize);
	if(imageWidth > imageHeight) return vec2(gridSize, ceil(1.0 * imageHeight * gridSize / imageWidth));
	return vec2(ceil(1.0 * imageWidth * gridSize / imageHeight), gridSize);
}

void initPoints()
{
	unsigned char* imageData = initImage();

	// Save the image as JPG
	// stbi_write_jpg("output_image.jpg", imageWidth, imageHeight, channels, imageData, 100); // Quality set to 100
	vec2 grid = determineGridSize();
	
	int imageWidthPerPoint = imageWidth / grid.y;
	int imageHeightPerPoint = imageHeight / grid.x;

	std::cout << "per point width and height: " << imageWidthPerPoint << ", " << imageHeightPerPoint << std::endl; 

	if(debug == false){
		std::cout << "Grid size: " << grid.x << ", " << grid.y << std::endl;
	}

	if(channels < 3 && channels > 4){
		std::cout << "Channels: " << channels << std::endl;
		std::cout << "Code does not work for channels less than three";
		exit(1);
	}
	std::cout << "Number of channels: " << channels << std::endl;

	for(int i = 0 ; (i < imageWidth) && ( (i/imageWidthPerPoint) < grid.x); i = i + imageWidthPerPoint){
		for(int j = 0 ; (j < imageHeight) && ((j/imageHeightPerPoint) < grid.y); j = j + imageHeightPerPoint){
 			
			float avgColor[4] = {0, 0, 0, 0};
			int count = 0;
			// std::cout << std::endl;
			for(int l = j; l < j + imageHeightPerPoint && l < imageHeight; l = l + 1 ){
				for(int k = i ; k < i + imageWidthPerPoint && k < imageWidth; k = k + 1 ){
					// Calculate the index for the current pixel
					int actualRow = imageHeight - 1 - l;  // Invert Y-axis
					int index = (actualRow * imageWidth + k) * channels;

					// int actualCol = imageWidth - 1 - k;
					// int index = ( actualCol * imageHeight + l) * channels;
					// std::cout << index << std::endl;
					// Accumulate the pixel's color channels
					avgColor[0] += imageData[index];       // Red
					avgColor[1] += imageData[index + 1];   // Green
					avgColor[2] += imageData[index + 2];   // Blue
					if(channels == 4){
						avgColor[3] += imageData[index + 3];
					}

					++count;
				
				}	
				
			}

			// Calculate the average color
			if (count > 0) {
				
				colors.push_back(avgColor[0] * 1.0 / count);
				colors.push_back(avgColor[1] * 1.0 / count);
				colors.push_back(avgColor[2] * 1.0 / count);
				if(channels == 3){
					colors.push_back(255);
				}
				else{
					colors.push_back(avgColor[3] * 1.0 / count);

				}

				
        	}
		}
	}
	if(grid.x > grid.y) imgRange.y = int( grid.y * imgRange.x / grid.x );
	else if(grid.y > grid.x) imgRange.x = int( grid.x * imgRange.y / grid.y );
	// Generate the p oints for a 64x64 grid
	int colorIndex = 0;
	for (int i = 0; i < grid.x; ++i) {
		for (int j = 0; j < grid.y; ++j) {
			// Normalize the grid coordinates to be between -1.0 and 1.0
			GLfloat x = (2.0f * imgRange.x * i) / (grid.x - 1) - imgRange.x;  // x goes from -1.0 to 1.0
			GLfloat y = (2.0f * imgRange.y * j) / (grid.y - 1) - imgRange.y;  // y goes from -1.0 to 1.0
			points.push_back(x);
			points.push_back(y);
			points.push_back(pointsZ);  // z is 0 for a flat grid

			// color
			
			points.push_back(colors[colorIndex++] / 255.0);  // R
			points.push_back(colors[colorIndex++] / 255.0);  // G
			points.push_back(colors[colorIndex++] / 255.0);  // B
			points.push_back(colors[colorIndex++] / 255.0);
			// points.push_back(imageData[colorIndex++] / 255.0);  // R
			// points.push_back(imageData[colorIndex++] / 255.0);  // G
			// points.push_back(imageData[colorIndex++] / 255.0);  // B
		}
	}

	pointCount = grid.x * grid.y;
	stbi_image_free(imageData);

	// Enable blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenBuffers(1, &vbuf);
	glGenVertexArrays(1, &vao);

	// Bind the VAO (Vertex Array Object)
	glBindVertexArray(vao);

	// Bind the VBO (Vertex Buffer Object) and upload the point data
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(GLfloat), points.data(), GL_STATIC_DRAW);

	// Define the vertex attribute (position) layout
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);
	// Enable the color attribute (location = 1)
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));  // Color attribute
	glEnableVertexAttribArray(1);
	// glPointSize(1.0f * width / gridSize);
	glPointSize(20.0f);

	// Unbind the VAO
	glBindVertexArray(0);

}



unsigned char* initImage()
{
	unsigned char* imageData = stbi_load("imageB.png", &imageWidth, &imageHeight, &channels, 0);


	if (!imageData) {
		std::cerr << "Failed to load image!" << std::endl;
		return 0;
	}
   

	return imageData;
	// get image width and height, divide by pointGrid size to find out how many pixels of the image should form a point color
	// int width, height;
	// unsigned char* imageData = SOIL_load_image("download_64x64.jpg", &imageWidth, &imageHeight, 0, SOIL_LOAD_RGB);

	// if (imageData) {
	// 	// Use the image data
	// 	SOIL_free_image_data(imageData);
	// } else {
	// 	std::cerr << "Image loading failed!" << std::endl;
	// }
	// return imageData;
}

void squash(vec2 P)
{
	P.x = P.x / width;
	P.y = 1 - ( P.y / height );

	// fov = 45.0f;
	float yRange = abs( camCoords.z - pointsZ ) * tan(fovAngle * M_PI / 180.0f);
	float xRange = yRange * (float)width / (float)height;

	vec2 clickedCoord = P * vec2(xRange, yRange) - vec2(xRange / 2.0f, yRange / 2.0f);

	if( debug == true ){
		// std::cout << P.x << " " << P.y << std::endl;
		// std::cout << clickedCoord.x << ", " << clickedCoord.y << std::endl;
	}

	// std::cout << "Squash" << std::endl;
	// std::cout << "Clicked:" << clickedCoord.x << ", " << clickedCoord.y << std::endl;

	// if(clickedCoord.x > 1 || clickedCoord.x < -1 || clickedCoord.y > 1 || clickedCoord.y < -1){
	// 	std::cout << "out of bound" << std::endl;
	// 	return;
	// }


	for( int i = 0 ; i < points.size() ; i += 7 ){
		// alpha / glm::pow( glm::distance( clickedCoord, vec2(points[i], points[i+1]) ), 2);
		vec2 tPoint = vec2(points[i], points[i+1]);
		// std::cout << points[i] << ", " << points[i+1] << std::endl;
		vec2 result = tPoint - clickedCoord;
		// std::cout << result.x << ", " << result.y << std::endl;
		float distance = 0.1 / ( result.x * result.x + result.y * result.y );
		// std::cout << distance << std::endl;
		result = tPoint + distance * glm::normalize(result);
		// std::cout << "Result: " << result.x << ", " << result.y << std::endl;
		points[i] = result.x;
		points[i+1] = result.y;
		// break;

	}

	// // Bind the VBO (Vertex Buffer Object) and upload the point data
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(GLfloat), points.data(), GL_STATIC_DRAW);

	// glutPostRedisplay();

	
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
		mat4 proj = perspective(fovAngle, aspect, 0.1f, 100.0f);
		// Create view transformation matrix
		mat4 view = translate(mat4(1.0f), vec3(0.0, 0.0, -camCoords.z)); 
		mat4 rot = rotate(mat4(1.0f), radians(camCoords.y), vec3(1.0, 0.0, 0.0));
		rot = rotate(rot, radians(camCoords.x), vec3(0.0, 1.0, 0.0));
		xform = proj * view * rot;
		
		glBindVertexArray(vao);

		// Send the transformation matrix to the shader
		glUniformMatrix4fv(uniXform, 1, GL_FALSE, glm::value_ptr(xform));
		glUniform1f(uSigma, 0.8f);



		switch (viewmode) {
		case VIEWMODE_SQUARE:
			glUniform1i(uRenderDiscLoc, 0);  // Enable disc rendering
			break;
		case VIEWMODE_GAUSS:
			glUniform1i(uRenderDiscLoc, 2);
			break;

		case VIEWMODE_DISC: {
			glUniform1i(uRenderDiscLoc, 1);  // Enable disc rendering
			break; }
		}

		
		// Draw points
		glDrawArrays(GL_POINTS, 0, pointCount);  

		// Unbind the VAO
		glBindVertexArray(0);

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

// Function to handle special key events (for arrow keys)
void keyPressed(int key, int x, int y) {	
		
	vec2 mouseDelta = vec2(0, 0);
	float factor = 1.0;
    switch (key) {
        case GLUT_KEY_UP:
			mouseDelta.y += factor;
            break;
        case GLUT_KEY_DOWN:
			mouseDelta.y -= factor;
            break;
        case GLUT_KEY_LEFT:
			mouseDelta.x += factor;
            break;
        case GLUT_KEY_RIGHT:
			mouseDelta.x -= factor;
            break;
        default:
            break;
    }

	float rotScale = glm::min(width / 450.0f, height / 270.0f);
	vec2 newAngle = camOrigin + mouseDelta / rotScale;
	newAngle.y = clamp(newAngle.y, -90.0f, 90.0f);
	
	camCoords.x += newAngle.x;
	camCoords.y += newAngle.y;
	while (camCoords.x > 180.0f) camCoords.x -= 360.0f;
	while (camCoords.y < -180.0f) camCoords.y += 360.0f;
	glutPostRedisplay();

}

void mouseBtn(int button, int state, int x, int y) {
	if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
		// camRot = true;
		// camOrigin = vec2(camCoords);
		// mouseOrigin = vec2(x, y);

		squash(vec2(x, y));
		glutPostRedisplay();
	}
	if (state == GLUT_UP && button == GLUT_LEFT_BUTTON) {
		// Deactivate rotation
		camRot = false;
	}
	if (button == 3) {
		camCoords.z = clamp(camCoords.z - 0.1f, 0.1f, 500.0f);
		glutPostRedisplay();
	}
	if (button == 4) {
		camCoords.z = clamp(camCoords.z + 0.1f, 0.1f, 500.0f);
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
		viewmode = (viewmode + 1) % 3;
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
	uRenderDiscLoc = 0;
	if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
	if (vbuf) { glDeleteBuffers(1, &vbuf); vbuf = 0; }
	vcount = 0;
}
