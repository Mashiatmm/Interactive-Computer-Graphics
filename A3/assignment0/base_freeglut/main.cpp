#include <iostream>
#include <random>
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
GLuint geometryPassShader, ssaoPassShader, blurShader, LightingPassShader, simpleQuadShader;
// GLuint shader;			// Shader program
// GLuint uniXform;		// Shader location of xform mtx
GLuint vao;				// Vertex array object
GLuint vbuf;			// Vertex buffer
GLsizei vcount;			// Number of vertices
Mesh* mesh;				// Mesh loaded from .obj file
std::vector<Mesh*> meshList;
unsigned int numObj;
unsigned int gBuffer;
unsigned int gPosition, gNormal, gAlbedo, rboDepth;
unsigned int ssaoFBO, ssaoBlurFBO;
unsigned int ssaoColorBuffer, ssaoColorBufferBlur;
unsigned int noiseTexture;
std::vector<glm::vec3> ssaoKernel;
unsigned int quadVAO = 0;
unsigned int quadVBO = 0;
// lighting info
// -------------
glm::vec3 lightPos, lightColor;
float k, m;
int n;
char activate;

// Camera state
vec3 camCoords;			// Spherical coordinates (theta, phi, radius) of the camera
bool camRot;			// Whether the camera is currently rotating
vec2 camOrigin;			// Original camera coordinates upon clicking
vec2 mouseOrigin;		// Original mouse coordinates upon clicking


bool debug;

// Constants
const int MENU_VIEWMODE = 0;		// Toggle view mode
const int MENU_EXIT = 1;			// Exit application

// Initialization functions
void initState();
void initGLUT(int* argc, char** argv);
void initOpenGL();
void initTriangle();
void initGBuffer();
void initSSAOBuffer();
float lerp(float a, float b, float f);
glm::mat4 getCamViewMatrix();
void renderQuad();

// Callback functions
void display();
void reshape(GLint width, GLint height);
void keyRelease(unsigned char key, int x, int y);
void keyPressed(unsigned char key, int x, int y);
void SpecialKeyPressed(int key, int x, int y);
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
		initGBuffer();
		initSSAOBuffer();

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

float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

void initState() {
	// Initialize global state
	width = 0;
	height = 0;
	geometryPassShader = 0;
	ssaoPassShader = 0;
	blurShader = 0;
	LightingPassShader = 0;
	simpleQuadShader = 0;
	k = 1;
	m = 1;
	n = 64;
	activate = '\0';
	numObj = 2;
	vao = 0;
	vbuf = 0;
	vcount = 0;
	mesh = NULL;
	lightPos = glm::vec3(2.0, 4.0, -2.0);
	lightColor = glm::vec3(1.0, 1.0, 1.0);

	camCoords = vec3(0.0, 0.0, 5.0);
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
	glutKeyboardFunc(keyPressed);
	glutSpecialFunc(SpecialKeyPressed);
	glutIdleFunc(idle);
	glutCloseFunc(cleanup);
}

void initOpenGL() {
	// Set clear color and depth
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	// Compile and link shader program
	vector<GLuint> shaders;
	shaders.push_back(compileShader(GL_VERTEX_SHADER, "sh_v.glsl"));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "sh_f.glsl"));
	geometryPassShader = linkProgram(shaders);
	// Release shader sources
	for (auto s = shaders.begin(); s != shaders.end(); ++s)
		glDeleteShader(*s);
	shaders.clear();


	shaders.push_back(compileShader(GL_VERTEX_SHADER, "ssao_v.glsl"));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "ssao_f.glsl"));
	ssaoPassShader = linkProgram(shaders);

	// Release shader sources
	for (auto s = shaders.begin(); s != shaders.end(); ++s)
		glDeleteShader(*s);
	shaders.clear();


	shaders.push_back(compileShader(GL_VERTEX_SHADER, "ssao_v.glsl"));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "ssao_f_blur.glsl"));
	blurShader = linkProgram(shaders);
	// Release shader sources
	for (auto s = shaders.begin(); s != shaders.end(); ++s)
		glDeleteShader(*s);
	shaders.clear();

	shaders.push_back(compileShader(GL_VERTEX_SHADER, "ssao_v.glsl"));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "ssao_f_lighting.glsl"));
	LightingPassShader = linkProgram(shaders);
	// Release shader sources
	for (auto s = shaders.begin(); s != shaders.end(); ++s)
		glDeleteShader(*s);
	shaders.clear();

	// Locate uniforms
	// uniXform = glGetUniformLocation(shader, "xform");

	vector<GLuint> quadShader;
	quadShader.push_back(compileShader(GL_VERTEX_SHADER, "ssao_v.glsl"));
	quadShader.push_back(compileShader(GL_FRAGMENT_SHADER, "simple_texture_fs.glsl"));
	simpleQuadShader = linkProgram(quadShader);
	for (auto s = quadShader.begin(); s != quadShader.end(); ++s)
		glDeleteShader(*s);
	quadShader.clear();
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

void initGBuffer()
{
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void initSSAOBuffer()
{
    glGenFramebuffers(1, &ssaoFBO);  
	glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    // SSAO color buffer
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Framebuffer not complete!" << std::endl;
    // and blur stage
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // generate sample kernel
    // ----------------------
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / n;

        // scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // generate noise texture
    // ----------------------
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 64; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }
	glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	

    // shader configuration
    // --------------------
    glUseProgram(LightingPassShader);
	glUniform1i(glGetUniformLocation(LightingPassShader, "gPosition"), 0); 
	glUniform1i(glGetUniformLocation(LightingPassShader, "gNormal"), 1); 
	glUniform1i(glGetUniformLocation(LightingPassShader, "gAlbedo"), 2);
	glUniform1i(glGetUniformLocation(LightingPassShader, "ssao"), 3); 
 

	glUseProgram(ssaoPassShader);
	
    glUseProgram(blurShader);
	glUniform1i(glGetUniformLocation(blurShader, "ssaoInput"), 0); 
	glUseProgram(0); // NOT SURE - ADDED BY MASHIAT
}


// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------

void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// returns the view matrix calculated using Euler Angles and the LookAt Matrix
glm::mat4 getCamViewMatrix()
{
	glm::vec3 Position = glm::vec3(
		camCoords.z * sin(radians(camCoords.y)) * sin(radians(camCoords.x))
	);
	glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 Front = glm::normalize(-Position);
	return glm::lookAt(Position, Position + Front, Up);
}

void visualizeGBuffer(GLuint texture) {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind the simple shader
    glUseProgram(simpleQuadShader);

    // Bind the texture you want to visualize
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(simpleQuadShader, "gBufferTexture"), 0);

    // Render a fullscreen quad to display the texture
    renderQuad();

    // Swap buffers
    glutSwapBuffers();
}



void display() {
	try {
		
		// Bind the G-buffer framebuffer (gBuffer) for geometry pass
	
		mat4 xform;
		float aspect = (float)width / (float)height;
		// Create perspective projection matrix
		mat4 proj = perspective(45.0f, aspect, 0.1f, 100.0f);
		// Create view transformation matrix
		mat4 view = translate(mat4(1.0f), vec3(0.0, 0.0, -camCoords.z)); 
		mat4 rot = rotate(mat4(1.0f), radians(camCoords.y), vec3(1.0, 0.0, 0.0));
		rot = rotate(rot, radians(camCoords.x), vec3(0.0, 1.0, 0.0));
		// xform = proj * view * rot;

		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(geometryPassShader);

		// Concatenate all transformations and upload to shader
		// xform = xform * fixBB;

		// Use geometry pass shader program

		// Upload uniform matrices
		glUniformMatrix4fv(glGetUniformLocation(geometryPassShader, "view"), 1, GL_FALSE, value_ptr(view * rot));
		glUniformMatrix4fv(glGetUniformLocation(geometryPassShader, "projection"), 1, GL_FALSE, value_ptr(proj));
		
		// room cube
        glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0, 7.0f, 0.0f));
		model = glm::scale(model, glm::vec3(7.5f, 7.5f, 7.5f));
		glUniformMatrix4fv(glGetUniformLocation(geometryPassShader, "model"), 1, GL_FALSE, value_ptr(model));
		glUniform1i(glGetUniformLocation(geometryPassShader, "invertedNormals"), 1); 
		if(!mesh) mesh = new Mesh("models/cube.obj");
		mesh->draw();
		// glUniformMatrix4fv(glGetUniformLocation(geometryPassShader, "xform"), 1, GL_FALSE, value_ptr(xform));
		glUniform1i(glGetUniformLocation(geometryPassShader, "invertedNormals"), 0); 
		// Load and prepare mesh if not already loaded
		for(int i = 1 ; i <= numObj; i++){

			if(meshList.size() < i) meshList.push_back( new Mesh("models/bunny2.obj") );

			// Scale and center mesh using bounding box
			pair<vec3, vec3> meshBB = meshList[i-1]->boundingBox();
		
			mat4 fixBB = scale(mat4(1.0f), vec3(1.0f / length(meshBB.second - meshBB.first)));
			fixBB = glm::translate(fixBB, - (meshBB.first + meshBB.second) / 2.0f);
    		fixBB = glm::translate(fixBB, vec3( (i-1) * 2.0f, 0.0f, (i-1) * 2.0f)); // Adjust spacing by changing `2.0f` if needed
			glUniformMatrix4fv(glGetUniformLocation(geometryPassShader, "model"), 1, GL_FALSE, value_ptr(fixBB));

			// Draw the mesh
			meshList[i-1]->draw();

		}
		

		// Unbind framebuffer kernelto switch back to the default framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		assert(glGetError() == GL_NO_ERROR);



		// 2. generate SSAO texture
        // ------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(ssaoPassShader);
		// Send kernel + rotation 
		for (unsigned int i = 0; i < 64; ++i){
			    std::string uniformName = "samples[" + std::to_string(i) + "]";

			glUniform3fv(glGetUniformLocation(ssaoPassShader, uniformName.c_str()), 1, &ssaoKernel[i][0]); 
		}
		glUniformMatrix4fv(glGetUniformLocation(ssaoPassShader, "projection"), 1, GL_FALSE, value_ptr(proj));
		glUniform1f(glGetUniformLocation(ssaoPassShader, "m"), m);
		glUniform1f(glGetUniformLocation(ssaoPassShader, "k"), k);
		glUniform1i(glGetUniformLocation(ssaoPassShader, "n"), n);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		renderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
		assert(glGetError() == GL_NO_ERROR);
		// visualizeGBuffer(ssaoColorBuffer);

		// // Revert state
		// // glUseProgram(0);

		// 3. blur SSAO texture to remove noise
        // ------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(blurShader);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
		renderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
		assert(glGetError() == GL_NO_ERROR);

		// visualizeGBuffer(ssaoColorBufferBlur);


		// 4. lighting pass: traditional deferred Blinn-Phong lighting with added screen-space ambient occlusion
        // -----------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(LightingPassShader);
        // send light relevant uniforms
        glm::vec3 lightPosView = glm::vec3(getCamViewMatrix() * glm::vec4(lightPos, 1.0));
        glUniform3fv(glGetUniformLocation(LightingPassShader, "light.Position"), 1, &lightPosView[0]); 
		glUniform3fv(glGetUniformLocation(LightingPassShader, "light.Color"), 1, &lightColor[0]); 

        // Update attenuation parameters
        const float linear    = 0.09f;
        const float quadratic = 0.032f;
		glUniform1f(glGetUniformLocation(LightingPassShader, "light.Linear"), linear); 
		glUniform1f(glGetUniformLocation(LightingPassShader, "light.Quadratic"), quadratic);
	 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glActiveTexture(GL_TEXTURE3); // add extra SSAO texture to lighting pass
        glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
		renderQuad();
		assert(glGetError() == GL_NO_ERROR);

		// Display the back buffer
		glUseProgram(0);
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

void keyPressed(unsigned char key, int x, int y)
{
	switch(key)
	{
	case 'k':
		activate = 'k';
		// glutPostRedisplay();
		break;
	case 'm':
		activate = 'm';
		// glutPostRedisplay();
		break;
	case 'n':
		activate = 'n';
		break;
	case 'r':
		k = 1;
		m = 1;
		n = 64;
		activate = '\0';
		glutPostRedisplay();
		break;
	}
}

// Function to handle special key events (for arrow keys)
void SpecialKeyPressed(int key, int x, int y) {	
		
	
    switch (key) {
        case GLUT_KEY_UP:
			if(activate == 'k') k += 0.1;
			else if(activate == 'm') m += 0.1;
			else if(activate == 'n') { if(n < 128) n += 8; }
			// activate = '\0';
            break;
        case GLUT_KEY_DOWN:
			if(activate == 'k') k -= 0.1;
			else if(activate == 'm') m -= 0.1;
			else if(activate == 'n') { if(n >= 8) n -= 8; }

			// activate = '\0';
            break;
		case GLUT_KEY_LEFT:
			if(numObj < 10) numObj += 1;
			break; 
		case GLUT_KEY_RIGHT:
			if(numObj > 1) numObj -= 1;
			break;       
    }

	glutPostRedisplay();

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
		glutPostRedisplay();	// Tell GLUT to redraw the screen
		break;

	case MENU_EXIT:
		glutLeaveMainLoop();
		break;
	}
}

void cleanup() {
	// Release all resources
	if (geometryPassShader) { glDeleteProgram(geometryPassShader); geometryPassShader = 0; }
	if (ssaoPassShader) { glDeleteProgram(ssaoPassShader); ssaoPassShader = 0; }
	if (blurShader) { glDeleteProgram(blurShader); blurShader = 0; }
	if (LightingPassShader) { glDeleteProgram(LightingPassShader); LightingPassShader = 0; }
	if (simpleQuadShader) { glDeleteProgram(simpleQuadShader); simpleQuadShader = 0; }

	// uniXform = 0;
	if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
	if (vbuf) { glDeleteBuffers(1, &vbuf); vbuf = 0; }
	vcount = 0;
	if (mesh) { delete mesh; mesh = NULL; }
	for (auto s = meshList.begin(); s != meshList.end(); ++s)
		delete *s;
	meshList.clear();
}
