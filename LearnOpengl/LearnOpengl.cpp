#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "model.h"

#include <iostream>
#include "Shader.h"
#include <random>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 1024;

bool startPress = true;
float roll = 0.0;
float pitch = 0.0;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;

// camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 8.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// lighting
glm::vec3 lightDir(-8.0f, 8.0f, 8.0f);

GLfloat lerp(GLfloat a, GLfloat b, GLfloat f)
{
    return a + f * (b - a);
}


int main()
{

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
 
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_DEPTH_TEST);

    Shader shadow("shadow_mapping_depth.vs", "shadow_mapping_depth.fs");
    Shader ourShader("model_loading.vs", "model_loading.fs");
    Shader debugDepthQuad("debug_quad.vs", "debug_quad.fs");

    Shader shaderGeometryPass("ssao_geometry.vs", "ssao_geometry.fs");
    Shader shaderSSAO("ssao.vs", "ssao.fs");
    Shader shaderSSAOBlur("ssao.vs", "ssao_blur.fs");


    Model ourModel("assets/backpack/backpack.obj");

    //shadow map
    unsigned int FBO;
    glGenFramebuffers(1, &FBO);

    const unsigned int SHADOW_WIDTH = 2048;
    const unsigned int SHADOW_HEIGHT = 2048;
    unsigned int shadowMap;
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    //gPositionDepth
    GLuint gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    GLuint gPositionDepth;
    // - Position + linear depth color buffer
    glGenTextures(1, &gPositionDepth);
    glBindTexture(GL_TEXTURE_2D, gPositionDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPositionDepth, 0);
    GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);

    // Also create framebuffer to hold SSAO processing stage 
    GLuint ssaoFBO, ssaoBlurFBO;
    glGenFramebuffers(1, &ssaoFBO);  glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    GLuint ssaoColorBuffer, ssaoColorBufferBlur;
    // - SSAO color buffer
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Framebuffer not complete!" << std::endl;
    // - and blur stage
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Sample kernel
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (GLuint i = 0; i < 64; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        GLfloat scale = GLfloat(i) / 64.0;

        // Scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // Noise texture
    std::vector<glm::vec3> ssaoNoise;
    for (GLuint i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }
    GLuint noiseTexture; glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    ourShader.use();
    ourShader.setInt("shadowMap", 3);

    // Set samplers
    shaderSSAO.use();
    shaderSSAO.setInt("gPositionDepth", 3);
    shaderSSAO.setInt("texNoise", 4);

    glm::mat4 projection = glm::perspective(45.0f, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 lightProjection, lightView;
    glm::mat4 lightSpaceMatrix;
    float near_plane = -15.0f, far_plane = 15.0f;
    lightProjection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, near_plane, far_plane);
    lightView = glm::lookAt(lightDir, glm::vec3(0.0, 0.0, 0.0), glm::vec3(-1.0, 1.0, -1.0));
    lightSpaceMatrix = lightProjection * lightView;


    while (!glfwWindowShouldClose(window))
    {

        processInput(window);


        // render
        // ------

        //shadow map
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glClear(GL_DEPTH_BUFFER_BIT);
        shadow.use();
        shadow.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(roll), glm::vec3(1.0f, 0.0f, 0.0f));
        model = model * glm::rotate(model, glm::radians(pitch), glm::vec3(0.0f, 1.0f, 0.0f));
        shadow.setMat4("model", model);
        ourModel.Draw(shadow);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //  Geometry Pass: render scene's geometry/color data into gbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderGeometryPass.use();
        shaderGeometryPass.setMat4("projection", projection);
        shaderGeometryPass.setMat4("view", view);
        shaderGeometryPass.setMat4("model", model);
        ourModel.Draw(shaderGeometryPass);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Create SSAO texture
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        shaderSSAO.use();
        shaderSSAO.setMat4("view", view);
        shaderSSAO.setMat4("model", model);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gPositionDepth);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        // Send kernel + rotation 
        for (GLuint i = 0; i < 64; ++i)
            glUniform3fv(glGetUniformLocation(shaderSSAO.ID, ("samples[" + std::to_string(i) + "]").c_str()), 1, &ssaoKernel[i][0]);
        shaderSSAO.setMat4("projection", projection);
        ourModel.Draw(shaderSSAO);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Blur SSAO texture to remove noise
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        shaderSSAOBlur.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        ourModel.Draw(shaderSSAOBlur);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //light pass
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ourShader.use();

        // view/projection transformations
        ourShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("lightDir", normalize(lightDir));
        ourShader.setVec3("viewPos", cameraPos);
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        ourShader.setMat4("model", model);
        // render the loaded model
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
        ourModel.Draw(ourShader);
    
        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}


void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (!startPress) {

        float xoffset = xpos - lastX;
        float yoffset = ypos - lastY;
        lastX = xpos;
        lastY = ypos;

        float ratio = 0.3f;
        pitch += xoffset * ratio;
        roll += yoffset * ratio;

    }

}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if (startPress) {
            glfwGetCursorPos(window, &xpos, &ypos);
            lastX = static_cast<float>(xpos);
            lastY = static_cast<float>(ypos);
            startPress = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        if (!startPress) {
            startPress = true;

        }
    }

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{

    glViewport(0, 0, width, height);
}