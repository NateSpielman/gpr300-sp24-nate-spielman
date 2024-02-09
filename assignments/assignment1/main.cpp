#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>
#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>
#include <ns/framebuffer.h>

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

ew::Camera camera;
ew::CameraController cameraController;

struct Material {
    float Ka = 1.0;
    float Kd = 0.5;
    float Ks = 0.5;
    float Shininess = 128;
}material;

struct ChromaticAberration {
    float rOffset = 0.005;
    float gOffset = 0.0;
    float bOffset = -0.005;
    int effectOn = 1;
}chromaticAberration;

int main() {
    GLFWwindow* window = initWindow("Assignment 1", screenWidth, screenHeight);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    GLuint rockTexture = ew::loadTexture("assets/Rock_Color.jpg");
    ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
    ew::Shader postProcessShader = ew::Shader("assets/postProcess.vert", "assets/postProcess.frag");
    ew::Model monkeyModel = ew::Model("assets/suzanne.obj");
    ew::Transform monkeyTransform;

    camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
    camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at center of the scene
    camera.aspectRatio = (float)screenWidth / screenHeight;
    camera.fov = 60.0f; //Vertical field of view, in degrees

    ns::Framebuffer framebuffer = ns::createFramebuffer(screenWidth, screenHeight, GL_RGB);

    unsigned int dummyVAO;
    glCreateVertexArrays(1, &dummyVAO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK); //Back face culling
    glEnable(GL_DEPTH_TEST); //Depth testing

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float time = (float)glfwGetTime();
        deltaTime = time - prevFrameTime;
        prevFrameTime = time;

        //First pass render to offscren frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
        glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Camera Controller
        cameraController.move(window, &camera, deltaTime);

        //Rotate model around Y axis
        monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

        //Bind rock texture to texture unit 0
        glBindTextureUnit(0, rockTexture);

        shader.use();
        shader.setInt("_MainTex", 0); //Make "_MainTex" sampler2D sample from the 2D texture bound to unit 0
        shader.setMat4("_Model", monkeyTransform.modelMatrix());
        shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
        shader.setVec3("_EyePos", camera.position);
        shader.setFloat("_Material.Ka", material.Ka);
        shader.setFloat("_Material.Kd", material.Kd);
        shader.setFloat("_Material.Ks", material.Ks);
        shader.setFloat("_Material.Shininess", material.Shininess);
        monkeyModel.draw(); //Draws the monkey model using current shader

        //Second pass render to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        postProcessShader.use();
        postProcessShader.setFloat("rOffset", chromaticAberration.rOffset);
        postProcessShader.setFloat("gOffset", chromaticAberration.gOffset);
        postProcessShader.setFloat("bOffset", chromaticAberration.bOffset);
        postProcessShader.setInt("effectOn", chromaticAberration.effectOn);

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
    if (ImGui::CollapsingHeader("Chromatic Aberration")) {
        ImGui::SliderFloat("R", &chromaticAberration.rOffset, 0.0f, 1.0f);
        ImGui::SliderFloat("G", &chromaticAberration.gOffset, 0.0f, 1.0f);
        ImGui::SliderFloat("B", &chromaticAberration.bOffset, 0.0f, 1.0f);
        ImGui::SliderInt("Toggle Effect", &chromaticAberration.effectOn, 0, 1);

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

