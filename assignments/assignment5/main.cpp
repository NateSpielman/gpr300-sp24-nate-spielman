#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>
#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>
#include <ew/procGen.h>
#include <ns/framebuffer.h>
#include <ns/shadowMap.h>
#include <ns/node.h>
#include <ns/hierarchy.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();

//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

//Camera
ew::Camera camera;
ew::CameraController cameraController;
ew::Camera shadowCamera;

//Shadow Map Variables
int shadowMapWidth = 2048;
int shadowMapHeight = 2048;
ns::ShadowMap shadowMap;

struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

struct Light {
	glm::vec3 lightDirection = glm::vec3(0.0f, -1.0f, 0.0f); //Light pointing straight down
	glm::vec3 lightColor = glm::vec3(1.0); //White light
	glm::vec3 ambientColor = glm::vec3(0.3, 0.4, 0.46);
}light;

float minBias = 0.005f;
float maxBias = 0.015f;

//Nodes
void SolveFK(ns::Hierarchy hierarchy);
void InitNodes();
void DrawNodes(ns::Hierarchy hierarchy, ew::Shader currentShader, ew::Model model);
void AnimNodes(ns::Hierarchy hierarchy);
const int NODECOUNT = 8;
ns::Node skeletonNodes[NODECOUNT];

int main() {
	GLFWwindow* window = initWindow("Assignment 5", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	GLuint rockTexture = ew::loadTexture("assets/Rock_Color.jpg");
	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader postProcessShader = ew::Shader("assets/postProcess.vert", "assets/postProcess.frag");
	ew::Shader depthOnlyShader = ew::Shader("assets/depthOnly.vert", "assets/depthOnly.frag");
	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");
	ew::Transform monkeyTransform;

	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(10, 10, 5));
	ew::Transform planeTransform;
	planeTransform.position = glm::vec3(0.0f, -2.0f, 0.0f);

	//Nodes
	InitNodes();
	//Hierarchy
	ns::Hierarchy hierarchy;
	hierarchy.nodes = skeletonNodes;
	hierarchy.nodeCount = NODECOUNT;
	
	//Main Camera
	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	//Shadow Camera
	shadowCamera.target = glm::vec3(0.0f, 0.0f, 0.0f);
	shadowCamera.orthographic = true;
	shadowCamera.orthoHeight = 15.0f;
	shadowCamera.nearPlane = 0.01f;
	shadowCamera.farPlane = 30.0f;
	shadowCamera.aspectRatio = 1.0f;

	//Create Framebuffer and shadow map
	ns::Framebuffer framebuffer = ns::createFramebuffer(screenWidth, screenHeight, GL_RGB);
	shadowMap = ns::createShadowMap(shadowMapWidth, shadowMapHeight);

	unsigned int dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		//RENDER
		//Shadow Map
		glCullFace(GL_FRONT);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.fbo);
		glViewport(0, 0, shadowMapWidth, shadowMapHeight);
		glClear(GL_DEPTH_BUFFER_BIT);
		
		shadowCamera.position = (shadowCamera.target - glm::normalize(light.lightDirection)) * 5.0f;
		depthOnlyShader.use();
		depthOnlyShader.setMat4("_ViewProjection", shadowCamera.projectionMatrix() * shadowCamera.viewMatrix());
		glCullFace(GL_BACK);
		//Nodes
		DrawNodes(hierarchy, depthOnlyShader, monkeyModel);
		depthOnlyShader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();

		//Offscreen Framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
		glViewport(0, 0, screenWidth, screenHeight);
		glClearColor(0.6f,0.8f,0.92f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Camera Controller
		cameraController.move(window, &camera, deltaTime);

		//Rotate model around Y axis
		//monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		//Bind textures to texture units
		glBindTextureUnit(0, rockTexture);
		glBindTextureUnit(1, shadowMap.depthMap);

		shader.use();
		shader.setInt("_MainTex", 0); //Make "_MainTex" sampler2D sample from the 2D texture bound to unit 0
		shader.setMat4("_Model", monkeyTransform.modelMatrix());
		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		shader.setVec3("_EyePos", camera.position);
		shader.setMat4("_LightViewProj", shadowCamera.projectionMatrix() * shadowCamera.viewMatrix());
		shader.setInt("_ShadowMap", 1);
		//Light
		shader.setVec3("_Light.LightDirection", light.lightDirection);
		shader.setVec3("_Light.LightColor", light.lightColor);
		shader.setVec3("_Light.AmbientColor", light.ambientColor);
		shader.setFloat("_MinBias", minBias);
		shader.setFloat("_MaxBias", maxBias);
		//Material 
		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("_Material.Shininess", material.Shininess);
		//monkeyModel.draw(); //Draws the monkey model using current shader

		shader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();

		//draw nodes
		AnimNodes(hierarchy);
		SolveFK(hierarchy);
		DrawNodes(hierarchy, shader, monkeyModel);

		//Scene
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		postProcessShader.use();
		glBindTextureUnit(0, framebuffer.colorBuffer[0]);
		glBindVertexArray(dummyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		drawUI();

		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void resetCamera(ew::Camera* camera, ew::CameraController* controller) {
	camera->position = glm::vec3(0, 0, 5.0f);
	camera->target = glm::vec3(0);
	controller->yaw = controller->pitch = 0;
}

void SolveFK(ns::Hierarchy hierarchy) {
	for (int i = 0; i < hierarchy.nodeCount; i++)
	{
		if (hierarchy.nodes[i].parentIndex == -1)
		{
			hierarchy.nodes[i].globalTransform = hierarchy.nodes[i].localTransform();
		}
		else {
			hierarchy.nodes[i].globalTransform = hierarchy.nodes[hierarchy.nodes[i].parentIndex].globalTransform * hierarchy.nodes[i].localTransform();
		}
	}
}

void InitNodes() {
	//Torso
	skeletonNodes[0] = ns::createNode(glm::vec3(0.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), -1);

	//Shoulder L
	skeletonNodes[1] = ns::createNode(glm::vec3(1.5f, 0.0f, 0.0f), glm::quat(0.75f, 0.0f, 0.75f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f), 0);
	//Elbow L
	skeletonNodes[2] = ns::createNode(glm::vec3(0.0f, 0.0f, 2.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(0.4f, 0.4f, 0.4f), 1);
	//Wrist L
	skeletonNodes[3] = ns::createNode(glm::vec3(0.0f, -5.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 2);

	//Shoulder R
	skeletonNodes[4] = ns::createNode(glm::vec3(-1.5f, 0.0f, 0.0f), glm::quat(0.75f, 0.0f, -0.75f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f), 0);
	//Elbow R
	skeletonNodes[5] = ns::createNode(glm::vec3(0.0f, 0.0f, 2.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(0.4f, 0.4f, 0.4f), 4);
	//Wrist R
	skeletonNodes[6] = ns::createNode(glm::vec3(0.0f, -5.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 5);

	//Head
	skeletonNodes[7] = ns::createNode(glm::vec3(0.0f, 1.5f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(0.7f, 0.7f, 0.7f), 0);
}

void DrawNodes(ns::Hierarchy hierarchy, ew::Shader currentShader, ew::Model model) {
	for (int i = 0; i < hierarchy.nodeCount; i++)
	{
		currentShader.setMat4("_Model", hierarchy.nodes[i].globalTransform);
		model.draw();
	}
}

void AnimNodes(ns::Hierarchy hierarchy) {
	//Torso
	hierarchy.nodes[0].rotation = glm::rotate(hierarchy.nodes[0].rotation, deltaTime, glm::vec3(0.0, -1.0, 0.0));
	hierarchy.nodes[0].position = hierarchy.nodes[0].rotation * glm::vec3(2.0f, 0.0f, 0.0f);

	//Shoulder L
	hierarchy.nodes[1].rotation = glm::rotate(hierarchy.nodes[1].rotation, deltaTime, glm::vec3(0.0, 0.0, -0.2));

	//Shoulder R
	hierarchy.nodes[4].rotation = glm::rotate(hierarchy.nodes[4].rotation, deltaTime, glm::vec3(0.0, 0.0, 0.2));

	//Head
	hierarchy.nodes[7].position.y = glm::mix(1.6f, 2.0f, sin((float)glfwGetTime() * 3.0f));
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::Button("Reset Camera")) {
		resetCamera(&camera, &cameraController);
	}
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

