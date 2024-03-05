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

ns::Framebuffer gBuffer;

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

int main() {
	GLFWwindow* window = initWindow("Assignment 3", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	GLuint rockTexture = ew::loadTexture("assets/Rock_Color.jpg");
	ew::Shader postProcessShader = ew::Shader("assets/postProcess.vert", "assets/postProcess.frag");
	ew::Shader geometryShader = ew::Shader("assets/geometryPass.vert", "assets/geometryPass.frag");
	ew::Shader deferredShader = ew::Shader("assets/deferredLit.vert", "assets/deferredLit.frag");
	ew::Shader depthOnlyShader = ew::Shader("assets/depthOnly.vert", "assets/depthOnly.frag");

	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");
	ew::Transform monkeyTransform;

	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(10, 10, 5));
	ew::Transform planeTransform;
	planeTransform.position = glm::vec3(0.0f, -2.0f, 0.0f);
	
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

	//Create Framebuffers and shadow map
	ns::Framebuffer framebuffer = ns::createFramebuffer(screenWidth, screenHeight, GL_RGB);
	gBuffer = ns::createGBuffer(screenWidth, screenHeight);
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
		depthOnlyShader.setMat4("_Model", monkeyTransform.modelMatrix());
		monkeyModel.draw();
		depthOnlyShader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();

		//Geometry pass
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.fbo);
		glViewport(0, 0, gBuffer.width, gBuffer.height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Bind rock texture before geometry shader
		glBindTextureUnit(0, rockTexture);

		geometryShader.use();
		geometryShader.setInt("_MainTex", 0);
		geometryShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		geometryShader.setMat4("_Model", monkeyTransform.modelMatrix());
		monkeyModel.draw();
		geometryShader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();

		//LIGHTING PASS
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
		glViewport(0, 0, framebuffer.width, framebuffer.height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		deferredShader.use();
		//Set the lighting uniforms for deferredShader
		deferredShader.setVec3("_EyePos", camera.position);
		deferredShader.setMat4("_LightViewProj", shadowCamera.projectionMatrix() * shadowCamera.viewMatrix());
		deferredShader.setVec3("_Light.LightDirection", light.lightDirection);
		deferredShader.setVec3("_Light.LightColor", light.lightColor);
		deferredShader.setVec3("_Light.AmbientColor", light.ambientColor);
		deferredShader.setFloat("_Material.Ka", material.Ka);
		deferredShader.setFloat("_Material.Kd", material.Kd);
		deferredShader.setFloat("_Material.Ks", material.Ks);
		deferredShader.setFloat("_Material.Shininess", material.Shininess);
		deferredShader.setFloat("_MinBias", minBias);
		deferredShader.setFloat("_MaxBias", maxBias);
		deferredShader.setInt("_ShadowMap", 3);

		//Bind g-buffer textures
		glBindTextureUnit(0, gBuffer.colorBuffer[0]);
		glBindTextureUnit(1, gBuffer.colorBuffer[1]);
		glBindTextureUnit(2, gBuffer.colorBuffer[2]);
		glBindTextureUnit(3, shadowMap.depthMap); //For shadow mapping

		glBindVertexArray(dummyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		cameraController.move(window, &camera, deltaTime);
		
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

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::Button("Reset Camera")) {
		resetCamera(&camera, &cameraController);
	}
	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}
	if (ImGui::CollapsingHeader("Light")) {
		ImGui::SliderFloat3("Direction", &light.lightDirection.x, -1.0f, 1.0f);
		ImGui::SliderFloat("Min Bias", &minBias, 0.001f, 0.05f);
		ImGui::SliderFloat("Max Bias", &maxBias, 0.001f, 0.05f);
	}
	ImGui::End();

	ImGui::Begin("Shadow Map");
	ImGui::BeginChild("Shadow Map");
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImGui::Image((ImTextureID)shadowMap.depthMap, windowSize, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::EndChild();
	ImGui::End();

	ImGui::Begin("GBuffers");
	ImVec2 texSize = ImVec2(gBuffer.width / 4, gBuffer.height / 4);
	for (size_t i = 0; i < 3; i++) {
		ImGui::Image((ImTextureID)gBuffer.colorBuffer[i], texSize, ImVec2(0, 1), ImVec2(1, 0));
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

