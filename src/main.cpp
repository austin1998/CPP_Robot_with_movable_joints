#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
//#include "body.h"

using namespace std;

float moveAngle = 0.1;
int upDown = 0;
int rotateAxis = 0;
bool bPress = false;
//all vars for robo
struct robo {
	shared_ptr<Shape>* obj;
	Shape obj2;
	robo* parent;
	vector <robo*> children;
	glm::vec3 translatToParent;
	glm::vec3 joint;
	glm::vec3 jointAngle;
	glm::vec3 translatToJoint;
	glm::vec3 scaleFactor;
	float moveX, moveY, moveZ;

};
// gives robo a linear order
struct roboT {
	roboT* next;
	robo* cur;
	roboT* prev;
};

roboT TTorso, THead, TUpLeftArm, TLowLeftArm, TUpRightArm, TLowRightArm, TUpLeftLeg, TLowLeftLeg, TUpRightLeg, TLowRightLeg, cur;

glm::vec3 getJoinAngle(robo* part) {
	return  part->translatToJoint - part->joint;
}
void travirseTree() {
	if (upDown == 1) {
		upDown = 0;
		cur = *cur.next;
		return;
	}
	if (upDown == 0) {
		upDown = 0;
		return;
	}
	if (upDown == -1) {
		upDown = 0;
		cur = *cur.prev;
		return;
	}
}

void drawRobo(robo* part, shared_ptr<MatrixStack> MV,shared_ptr<Program> prog, roboT* obj) {
	
	MV->pushMatrix();
	//translate part to joint
	MV->translate(part->translatToJoint + part->joint);
	//rotate part
	if (part == obj->cur) {
		if (bPress) {
			if (rotateAxis == 0) {
				part->moveX += moveAngle;
			}
			if (rotateAxis == 1) {
				part->moveY += moveAngle;
			}
			if (rotateAxis == 2) {
				part->moveZ += moveAngle;
			}
			bPress = false;
		}
	}
	MV->rotate(part->moveX, glm::vec3(1, 0, 0));
	MV->rotate(part->moveY, glm::vec3(0, 1, 0));
	MV->rotate(part->moveZ, glm::vec3(0, 0, 1));

	//translate part to proper location
	MV->translate(-part->joint);
	//draw children recursivly 
	if (part->children.size() > 0) {

		for (int i = 0; i < part->children.size(); i++) {
			drawRobo(part->children[i], MV, prog, obj);
		}
	}
	//scale parts to correct size
	MV->scale(part->scaleFactor);

	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, &MV->topMatrix()[0][0]);

	part->obj2 = **part->obj;
	//highlight selected part
	glColorMask(true, true, true, true);
	if (part == obj->cur) {
		glColorMask(false, true, false, true);
	}
	//actual draw function
	part->obj2.draw(prog);
	//reset color
	glColorMask(true, true, true, true);

	prog->unbind();
	MV->popMatrix();
	prog->bind();
}

void translateRoboJ(robo* part, float x, float y, float z) {
	part->translatToJoint = glm::vec3(x, y, z);
}

void sizeRobo(robo* part, float x, float y, float z) {
	part->scaleFactor = glm::vec3(x, y, z);
}


GLFWwindow *window; // Main application window
string RES_DIR = "./"; // Where data files live
shared_ptr<Program> prog;
shared_ptr<Shape> shape, head, torso, upLeftArm, lowLeftArm, upRightArm, lowRightArm, upLeftLeg, lowLeftLeg, upRightLeg, lowRightLeg;
robo rTorso, rHead, rUpLeftArm, rLowLeftArm, rUpRightArm, rLowRightArm, rUpLeftLeg, rLowLeftLeg, rUpRightLeg, rLowRightLeg;


static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) {
		//rotate x axis
		moveAngle = -.1;
	}
	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE) {
		moveAngle = .1;
	}
	if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		//rotate x axis
		bPress = true;
		rotateAxis = 0;
	}
	if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
		//rotate y axis
		bPress = true;
		rotateAxis = 1;
	}
	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		//rotate z axis
		bPress = true;
		rotateAxis = 2;
	}
	
	if (key == GLFW_KEY_PERIOD && action == GLFW_PRESS) {
		//treverse forward
		upDown = 1;
		travirseTree();
	}
	if (key == GLFW_KEY_COMMA && action == GLFW_PRESS) {
		//treverse forward
		upDown = -1;
		travirseTree();
	}
}

static void init()
{
	GLSL::checkVersion();

	// Set background color.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	// Initialize mesh.
	//torso
	torso = make_shared<Shape>();
	torso->loadMesh(RES_DIR + "cube.obj");
	torso->init();
	//create robo tree //put torso in tree
	rTorso.obj = &torso;
	rTorso.parent = NULL; 
	//
	translateRoboJ(&rTorso, 0, 0, 0);
	sizeRobo(&rTorso, 3, 4, 1);
	rTorso.joint = glm::vec3(0, 0, 0);

	head = make_shared<Shape>();
	head->loadMesh(RES_DIR + "cube.obj");
	head->init();
	//put head in tree
	//robo rHead;
	rHead.obj = &head;
	rHead.parent = &rTorso;
	rTorso.children.push_back(&rHead);
	//
	translateRoboJ(&rHead, 0, 2.5, 0);
	sizeRobo(&rHead, 1, 1, 1);
	rHead.joint = glm::vec3(0, -.5, 0);

	upLeftArm = make_shared<Shape>();
	upLeftArm->loadMesh(RES_DIR + "cube.obj");
	upLeftArm->init();
	//put upLeftArm in tree
	//
	rUpLeftArm.obj = &upLeftArm;
	rUpLeftArm.parent = &rTorso;
	rTorso.children.push_back(&rUpLeftArm);
	//
	translateRoboJ(&rUpLeftArm, 3, 1.5, 0);
	sizeRobo(&rUpLeftArm, 3, 1, 1);
	rUpLeftArm.joint = glm::vec3(-1.5, 0, 0);
	
	lowLeftArm = make_shared<Shape>();
	lowLeftArm->loadMesh(RES_DIR + "cube.obj");
	lowLeftArm->init();
	//put lowLeftArm in tree
	//
	rLowLeftArm.obj = &lowLeftArm;
	rLowLeftArm.parent = &rUpLeftArm;
	rUpLeftArm.children.push_back(&rLowLeftArm);
	//
	translateRoboJ(&rLowLeftArm, 2.5, 0, 0);
	sizeRobo(&rLowLeftArm, 2, .5, .5);
	rLowLeftArm.joint = glm::vec3(-1, 0, 0);

	upRightArm = make_shared<Shape>();
	upRightArm->loadMesh(RES_DIR + "cube.obj");
	upRightArm->init();
	//put upRightArm in tree
	//
	rUpRightArm.obj = &upRightArm;
	rUpRightArm.parent = &rTorso;
	rTorso.children.push_back(&rUpRightArm);
	//
	translateRoboJ(&rUpRightArm, -3, 1.5, 0);
	sizeRobo(&rUpRightArm, 3, 1, 1);
	rUpRightArm.joint = glm::vec3(1.5, 0, 0);

	lowRightArm = make_shared<Shape>();
	lowRightArm->loadMesh(RES_DIR + "cube.obj");
	lowRightArm->init();
	//put lowRightArm in tree
	//
	rLowRightArm.obj = &lowRightArm;
	rLowRightArm.parent = &rUpRightArm;
	rUpRightArm.children.push_back(&rLowRightArm);
	//
	translateRoboJ(&rLowRightArm, -2.5, 0, 0);
	sizeRobo(&rLowRightArm, 2, .5, .5);
	rLowRightArm.joint = glm::vec3(1, 0, 0);

	upLeftLeg = make_shared<Shape>();
	upLeftLeg->loadMesh(RES_DIR + "cube.obj");
	upLeftLeg->init();
	//put upLeftLeg in tree
	//
	rUpLeftLeg.obj = &upLeftLeg;
	rUpLeftLeg.parent = &rTorso;
	rTorso.children.push_back(&rUpLeftLeg);
	//
	translateRoboJ(&rUpLeftLeg, 1, -3.5, 0);
	sizeRobo(&rUpLeftLeg, 1, 3, 1);
	rUpLeftLeg.joint = glm::vec3(0, 1.5, 0);

	lowLeftLeg = make_shared<Shape>();
	lowLeftLeg->loadMesh(RES_DIR + "cube.obj");
	lowLeftLeg->init();
	//put lowLeftLeg in tree
	//
	rLowLeftLeg.obj = &lowLeftLeg;
	rLowLeftLeg.parent = &rUpLeftLeg;
	rUpLeftLeg.children.push_back(&rLowLeftLeg);
	//
	translateRoboJ(&rLowLeftLeg, 0, -2.5, 0);
	sizeRobo(&rLowLeftLeg, .5, -2, .5);
	rLowLeftLeg.joint = glm::vec3(0, 1, 0);

	upRightLeg = make_shared<Shape>();
	upRightLeg->loadMesh(RES_DIR + "cube.obj");
	upRightLeg->init();
	//put upRightLeg in tree
	//
	rUpRightLeg.obj = &upRightLeg;
	rUpRightLeg.parent = &rTorso;
	rTorso.children.push_back(&rUpRightLeg);
	//
	translateRoboJ(&rUpRightLeg, -1, -3.5, 0);
	sizeRobo(&rUpRightLeg, 1, 3, 1);
	rUpRightLeg.joint = glm::vec3(0, 1.5, 0);

	lowRightLeg = make_shared<Shape>();
	lowRightLeg->loadMesh(RES_DIR + "cube.obj");
	lowRightLeg->init();
	//put lowRightLeg in tree
	//
	rLowRightLeg.obj = &lowRightLeg;
	rLowRightLeg.parent = &rUpRightLeg;
	rUpRightLeg.children.push_back(&rLowRightLeg);
	//
	translateRoboJ(&rLowRightLeg, 0, -2.5, 0);
	sizeRobo(&rLowRightLeg, .5, 2, .5);
	rLowRightLeg.joint = glm::vec3(0, 1, 0);

	//make order for parts
	//next in list
	TTorso.next = &THead;
	THead.next = &TUpLeftArm;
	TUpLeftArm.next = &TLowLeftArm;
	TLowLeftArm.next = &TUpRightArm;
	TUpRightArm.next = &TLowRightArm;
	TLowRightArm.next = &TUpLeftLeg;
	TUpLeftLeg.next = &TLowLeftLeg;
	TLowLeftLeg.next = &TUpRightLeg;
	TUpRightLeg.next = &TLowRightLeg;
	TLowRightLeg.next = &TTorso;
	//cur in list
	TTorso.cur = &rTorso;
	THead.cur = &rHead;
	TUpLeftArm.cur = &rUpLeftArm;
	TLowLeftArm.cur = &rLowLeftArm;
	TUpRightArm.cur = &rUpRightArm;
	TLowRightArm.cur = &rLowRightArm;
	TUpLeftLeg.cur = &rUpLeftLeg;
	TLowLeftLeg.cur = &rLowLeftLeg;
	TUpRightLeg.cur = &rUpRightLeg;
	TLowRightLeg.cur = &rLowRightLeg;
	//prev in list
	TTorso.prev = &TLowRightLeg;
	TLowRightLeg.prev = &TUpRightLeg;
	TUpRightLeg.prev = &TLowLeftLeg;
	TLowLeftLeg.prev = &TUpLeftLeg;
	TUpLeftLeg.prev = &TLowRightArm;
	TLowRightArm.prev = &TUpRightArm;
	TUpRightArm.prev = &TLowLeftArm;
	TLowLeftArm.prev = &TUpLeftArm;
	TUpLeftArm.prev = &THead;
	THead.prev = &TTorso;
	cur = TTorso;

	// Initialize the GLSL program.
	prog = make_shared<Program>();
	prog->setVerbose(true);
	prog->setShaderNames(RES_DIR + "simple_vert.glsl", RES_DIR + "simple_frag.glsl");
	prog->init();
	prog->addUniform("P");
	prog->addUniform("MV");
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->setVerbose(false);
	
	// If there were any OpenGL errors, this will print something.
	// You can intersperse this line in your code to find the exact location
	// of your OpenGL error.
	GLSL::checkError(GET_FILE_LINE);
}

static void render()
{
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	float aspect = width/(float)height;
	glViewport(0, 0, width, height);

	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Create matrix stacks.
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	// Apply projection.
	P->pushMatrix();
	P->multMatrix(glm::perspective((float)(45.0*M_PI/180.0), aspect, 0.01f, 100.0f));
	// Apply camera transform.
	P->translate(0, 0, -20);
	// Draw mesh using GLSL.
	prog->bind();
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P->topMatrix()[0][0]);

	drawRobo(&rTorso, MV, prog, &cur);


	prog->unbind();

	// Pop matrix stacks.
	//MV->popMatrix();
	P->popMatrix();
	
	GLSL::checkError(GET_FILE_LINE);
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Please specify the resource directory." << endl;
		return 0;
	}
	RES_DIR = argv[1] + string("/");

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "Austin Offill", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

