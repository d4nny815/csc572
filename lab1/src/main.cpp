/*
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn (spline D. McGirr)
 */

#include <iostream>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include "stb_image.h"
#include "Bezier.h"
#include "Spline.h"
#include <chrono>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// constants
#define PI (3.1415927)
#define X_AXIS (vec3(1, 0, 0))
#define Y_AXIS (vec3(0, 1, 0))
#define Z_AXIS (vec3(0, 0, 1))

using namespace std;
using namespace glm;

class Application : public EventCallbacks {
public:

	WindowManager * windowManager = nullptr;

	// shader program
	std::shared_ptr<Program> prog;

	// shader program for textures
	std::shared_ptr<Program> texProg;

	// Objects
	shared_ptr<Shape> dragon;
	shared_ptr<Shape> armadillo;
	shared_ptr<Shape> sphere;

	// global data for ground plane - direct load constant defined CPU data to GPU (not obj)
	GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	int g_GiboLen;
	GLuint GroundVertexArrayID;

	// textures
	shared_ptr<Texture> ground_tex;
	// shared_ptr<Texture> texture1;	
	// shared_ptr<Texture> texture2;
	// shared_ptr<Texture> crate;
	shared_ptr<Texture> sky_tex;
	// shared_ptr<Texture> wood;

	// animation data
	float lightTrans = 0;

	//camera
	double g_phi, g_theta;
	vec3 view = vec3(0, 0, 1);
	vec3 strafe = vec3(1, 0, 0);
	vec3 g_eye = vec3(0, 1, 0);
	vec3 g_lookAt = vec3(0, 1, -4);
	const vec3 UP_VEC = vec3(0, 1, 0);
	const float MOUSE_SENS = .1;
	const float MOVEMENT_SENS = .5;
	const double MAX_YVIEW_ANG = M_PI_2 * 80 / 90;

	// light source
	float light_x = 0;
	float light_y = 0;
	float light_z = 0;

	void update_camera() {
		// calc new viewing point
		view.x = cos(g_theta) * cos(g_phi);
    	view.y = sin(g_phi); 
    	view.z = sin(g_theta) * cos(g_phi);
    	view = normalize(view);

		// strafe is orthogonal to view and up
    	strafe = normalize(cross(view, UP_VEC)); 

    	g_lookAt = g_eye + view;
	}

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
		// close window
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		// move camera
		if (key == GLFW_KEY_W && action == GLFW_PRESS) {
			g_eye += view * MOVEMENT_SENS;
			update_camera();
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS) {
			g_eye -= view * MOVEMENT_SENS;
			update_camera();
		}

		if (key == GLFW_KEY_A && action == GLFW_PRESS) {
			g_eye -= strafe * MOVEMENT_SENS;
			update_camera();
		}

		if (key == GLFW_KEY_D && action == GLFW_PRESS) {
			g_eye += strafe * MOVEMENT_SENS;
			update_camera();
		}

		// move light source
		if (key == GLFW_KEY_Q && action == GLFW_PRESS) { 
			lightTrans += 0.5;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS) {
			lightTrans -= 0.5;
		}

		// view geometry
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods) {
		double posX, posY;

		if (action == GLFW_PRESS) {
			 glfwGetCursorPos(window, &posX, &posY);
			 cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}

	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
		g_theta += deltaX * MOUSE_SENS;
    	g_phi += deltaY * MOUSE_SENS;
    	g_phi = clamp(g_phi, -MAX_YVIEW_ANG, MAX_YVIEW_ANG);

    	update_camera();
	}

	/* camera controls - do not change */
	void SetView(shared_ptr<Program>  shader) {
		glm::mat4 Cam = glm::lookAt(g_eye, g_lookAt, vec3(0, 1, 0));
		glUniformMatrix4fv(shader->getUniform("V"), 1, GL_FALSE, value_ptr(Cam));
  	}

	void resizeCallback(GLFWwindow *window, int width, int height) {
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory) {
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.72f, .84f, 1.06f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		g_theta = -PI / 2.0;

		// Initialize the GLSL program that we will use for local shading
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("MatAmb");
		prog->addUniform("MatDif");
		prog->addUniform("MatSpec");
		prog->addUniform("MatShine");
		prog->addUniform("lightPos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex"); //silence error


		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("flip");
		texProg->addUniform("Texture0");
		texProg->addUniform("MatShine");
		texProg->addUniform("lightPos");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");

		// read in a load the texture
		ground_tex = make_shared<Texture>();
  		ground_tex->setFilename(resourceDirectory + "/grass.jpg");
  		ground_tex->init();
  		ground_tex->setUnit(0);
  		ground_tex->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		sky_tex = make_shared<Texture>();
  		sky_tex->setFilename(resourceDirectory + "/cartoonSky.png");
  		sky_tex->init();
  		sky_tex->setUnit(1);
		sky_tex->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

	}

	void initGeom(const std::string& resourceDirectory) {
		// Initialize mesh
		// Load geometry
 		vector<tinyobj::shape_t> TOshapes;
 		vector<tinyobj::material_t> objMaterials;
 		string errStr;
 		bool rc;
		
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/dragon_vrip.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			dragon = make_shared<Shape>();
			dragon->createShape(TOshapes[0]);
			dragon->measure();
			dragon->init();
		}

		vector<tinyobj::shape_t> TOshapesB;
 		vector<tinyobj::material_t> objMaterialsB;
 		rc = tinyobj::LoadObj(TOshapesB, objMaterialsB, errStr, (resourceDirectory + "/Armadillo.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {	
			armadillo = make_shared<Shape>();
			armadillo->createShape(TOshapesB[0]);
			armadillo->measure();
			armadillo->init();
		}

		vector<tinyobj::shape_t> TOshapesC;
 		vector<tinyobj::material_t> objMaterialsC;
 		rc = tinyobj::LoadObj(TOshapesC, objMaterialsC, errStr, (resourceDirectory + "/sphereWTex.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {	
			sphere = make_shared<Shape>();
			sphere->createShape(TOshapesC[0]);
			sphere->measure();
			sphere->init();
		}

		// load in the ground plane
		initGround();
	}

	//directly pass quad for the ground to the GPU
	void initGround() {

		float g_groundSize = 20;
		float g_groundY = -0.25;

  		// A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
		float GrndPos[] = {
			-g_groundSize, g_groundY, -g_groundSize,
			-g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY, -g_groundSize
		};

		float GrndNorm[] = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};

		static GLfloat GrndTex[] = {
      		0, 0, // back
      		0, 1,
      		1, 1,
      		1, 0 };

      	unsigned short idx[] = {0, 1, 2, 0, 2, 3};

		//generate the ground VAO
      	glGenVertexArrays(1, &GroundVertexArrayID);
      	glBindVertexArray(GroundVertexArrayID);

      	g_GiboLen = 6;
      	glGenBuffers(1, &GrndBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndNorBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndTexBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

      	glGenBuffers(1, &GIndxBuffObj);
     	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
      	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
      }

      //code to draw the ground plane
     void drawGround(shared_ptr<Program> curS) {
     	curS->bind();
     	glBindVertexArray(GroundVertexArrayID);
     	ground_tex->bind(curS->getUniform("Texture0"));
		//draw the ground plane 
  		SetModel(vec3(0, -1, 0), 0, 0, 1, curS);
  		glEnableVertexAttribArray(0);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
  		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(1);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
  		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(2);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
  		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

   		// draw!
  		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
  		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

  		glDisableVertexAttribArray(0);
  		glDisableVertexAttribArray(1);
  		glDisableVertexAttribArray(2);
  		curS->unbind();
     }

    //helper function to pass material data to the GPU
	void SetMaterial(shared_ptr<Program> curS, int i) {

    	switch (i) {
    		case 0: //purple
    			glUniform3f(curS->getUniform("MatAmb"), 0.096, 0.046, 0.095);
    			glUniform3f(curS->getUniform("MatDif"), 0.96, 0.46, 0.95);
    			glUniform3f(curS->getUniform("MatSpec"), 0.45, 0.23, 0.45);
    			glUniform1f(curS->getUniform("MatShine"), 120.0);
    		break;
    		case 1: // pink
    			glUniform3f(curS->getUniform("MatAmb"), 0.063, 0.038, 0.1);
    			glUniform3f(curS->getUniform("MatDif"), 0.63, 0.38, 1.0);
    			glUniform3f(curS->getUniform("MatSpec"), 0.3, 0.2, 0.5);
    			glUniform1f(curS->getUniform("MatShine"), 4.0);
    		break;
    		case 2: 
    			glUniform3f(curS->getUniform("MatAmb"), 0.004, 0.05, 0.09);
    			glUniform3f(curS->getUniform("MatDif"), 0.04, 0.5, 0.9);
    			glUniform3f(curS->getUniform("MatSpec"), 0.02, 0.25, 0.45);
    			glUniform1f(curS->getUniform("MatShine"), 27.9);
    		break;
  		}
	}

	/* helper function to set model trasnforms */
  	void SetModel(vec3 trans, float rotY, float rotX, float sc, shared_ptr<Program> curS) {
  		mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
  		mat4 RotX = glm::rotate( glm::mat4(1.0f), rotX, vec3(1, 0, 0));
  		mat4 RotY = glm::rotate( glm::mat4(1.0f), rotY, vec3(0, 1, 0));
  		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
  		mat4 ctm = Trans*RotX*RotY*ScaleS;
  		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
  	}

	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
   	}

	void render(float frametime) {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Use the matrix stack for Lab 6
		float aspect = width/(float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

		Model->pushMatrix();
		Model->loadIdentity();
		Model->translate(vec3(0, -1, 0));


		// texture shader
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniform3f(texProg->getUniform("lightPos"), 0, 2.0, 0);
		glUniform1f(texProg->getUniform("MatShine"), 27.9);
		glUniform1i(texProg->getUniform("flip"), 0);
		SetView(texProg);
		
		// draw skybox
		Model->pushMatrix();
		Model->scale(10);
		setModel(texProg, Model);
		sky_tex->bind(texProg->getUniform("Texture0"));
		sphere->draw(texProg);
		Model->popMatrix();

		glUniform1i(texProg->getUniform("flip"), 1);
		drawGround(texProg);
		
		texProg->unbind();

		// blin-phong shader
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniform3f(prog->getUniform("lightPos"), light_x, light_y, light_z);
		glUniform1f(prog->getUniform("MatShine"), 27.9);
		SetView(prog);
		
		// draw dragon
		Model->pushMatrix();
		Model->translate(vec3(0, -1, -5));
		Model->scale(15);
		SetMaterial(prog, 1);
		setModel(prog, Model);
		dragon->draw(prog);
		Model->popMatrix();

		// draw armadillo
		Model->pushMatrix();
		Model->rotate(PI, Y_AXIS);
		Model->rotate(-PI/4, Y_AXIS);
		Model->translate(vec3(0, .5, 3));
		Model->scale(.01);
		SetMaterial(prog, 2);
		setModel(prog, Model);
		armadillo->draw(prog);
		Model->popMatrix();

		Model->popMatrix();

		prog->unbind();

		lightTrans = glfwGetTime();
		light_x = 5 * cos(lightTrans);
		light_z = 5 * sin(lightTrans);

		// Pop matrix stacks.
		Projection->popMatrix();
	}
};

int main(int argc, char *argv[]) {
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2) {
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(1280, 720);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	auto lastTime = chrono::high_resolution_clock::now();
	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// save current time for next frame
		auto nextLastTime = chrono::high_resolution_clock::now();

		// get time since last frame
		float deltaTime =
			chrono::duration_cast<std::chrono::microseconds>(
				chrono::high_resolution_clock::now() - lastTime)
				.count();
		// convert microseconds (weird) to seconds (less weird)
		deltaTime *= 0.000001;

		// reset lastTime so that we can calculate the deltaTime
		// on the next frame
		lastTime = nextLastTime;

		// Render scene.
		application->render(deltaTime);
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
