#include <glad/glad.h>
#include "mine/core/Application.hpp"
#include "mine/graphics/Renderer.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <cstdlib>

namespace mine {

Application::Application(int width, int height, const char* title) : m_lastFrameTime(glfwGetTime()), m_running(true) {
    if (!glfwInit()) {
        std::cerr << "❌ GLFW init failed\n";
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        std::cerr << "❌ Window creation failed\n";
        glfwTerminate();
        exit(1);
    }
    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "❌ GLAD init failed\n";
        exit(1);
    }

    std::cout << "✅ OpenGL " << glGetString(GL_VERSION) << "\n";
    m_renderer = new Renderer(width, height);
    
    // Start player at center of screen
    m_playerPos = {static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2.0f};
    m_cameraPos = m_playerPos;
    m_cameraTarget = m_playerPos;
}

Application::~Application() {
    delete m_renderer;
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::processInput() {
    float speed = 300.0f;
    float currentTime = glfwGetTime();
    float deltaTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_running = false;
    }
    
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        static bool hasShaken = false;
        if (!hasShaken) {
            shakeCamera(10.0f, 0.3f);
            hasShaken = true;
        }
    } else {
        static bool hasShaken = false;
        hasShaken = false;
    }
    
    // MOVEMENT: Use the SAME coordinate system as your renderer initialization
    // Your renderer initializes with: glm::ortho(0.0f, width, 0.0f, height, ...)
    // This means: X=0..width (left to right), Y=0..height (BOTTOM to TOP)
    // But GLFW mouse gives Y=0..height (TOP to bottom)
    
    if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_playerPos.x -= speed * deltaTime;
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_playerPos.x += speed * deltaTime;
    if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_playerPos.y += speed * deltaTime;  // UP = increase Y (toward top of screen)
    if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        m_playerPos.y -= speed * deltaTime;  // DOWN = decrease Y (toward bottom of screen)

    // MOUSE AIMING: Convert GLFW mouse coordinates to your world coordinates
    double mouseX, mouseY;
    int winWidth, winHeight;
    glfwGetCursorPos(m_window, &mouseX, &mouseY);
    glfwGetWindowSize(m_window, &winWidth, &winHeight);

    // GLFW: (0,0) = top-left, (width,height) = bottom-right
    // Your world: (0,0) = bottom-left, (width,height) = top-right
    // So we need to flip Y coordinate
    float worldMouseX = static_cast<float>(mouseX);
    float worldMouseY = static_cast<float>(winHeight) - static_cast<float>(mouseY);

    // BUT! We also need to account for camera offset
    // Camera position represents the CENTER of the screen
    worldMouseX += m_cameraPos.x - (winWidth / 2.0f);
    worldMouseY += m_cameraPos.y - (winHeight / 2.0f);

    // Calculate rotation from player to mouse
    float dx = worldMouseX - m_playerPos.x;
    float dy = worldMouseY - m_playerPos.y;
    m_playerRotation = atan2(dy, dx);
}

void Application::update(float deltaTime) {
    // Simple camera follow (no smoothing for testing)
    m_cameraPos = m_playerPos;

    // Camera shake decay
    if (m_cameraShakeDuration > 0.0f) {
        m_cameraShakeTimer += deltaTime;
        float progress = m_cameraShakeTimer / m_cameraShakeDuration;
        
        if (progress >= 1.0f) {
            m_cameraShakeDuration = 0.0f;
            m_cameraShakeIntensity = 0.0f;
            m_cameraShakeTimer = 0.0f;
        }
    }
}

void Application::render() {
    // Generate random shake
    Vector2 shakeOffset = {0.0f, 0.0f};
    if (m_cameraShakeIntensity > 0.0f) {
        shakeOffset.x = ((rand() % 100) - 50) / 100.0f * m_cameraShakeIntensity;
        shakeOffset.y = ((rand() % 100) - 50) / 100.0f * m_cameraShakeIntensity;
    }
    
    m_renderer->beginFrame();
    m_renderer->setCameraPosition(m_cameraPos, shakeOffset);
    
    // Draw everything
    m_renderer->drawQuad({0, 0}, {2000, 2000}, 0.3f, 0.3f, 0.3f); // Background
    m_renderer->drawQuad({200, 200}, {100, 100}, 0.2f, 0.6f, 0.8f); // Blue box
    m_renderer->drawQuad({0, 0}, {150, 80}, 0.7f, 0.3f, 0.1f); // Orange box
    m_renderer->drawQuad(m_playerPos, {64, 64}, 0.9f, 0.2f, 0.8f, m_playerRotation); // Player

    m_renderer->endFrame();
}

void Application::run() {
    std::cout << "\n🎮 XENOCIDE running! Move with WASD/arrow keys. Mouse to aim. ESC to exit.\n";
    std::cout << "   Press SPACE to shake camera\n\n";

    while (!glfwWindowShouldClose(m_window) && m_running) {
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;

        processInput();
        update(deltaTime);
        render();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    std::cout << "\n👋 Exited cleanly\n";
}

void Application::shakeCamera(float intensity, float duration) {
    m_cameraShakeIntensity = intensity;
    m_cameraShakeDuration = duration;
    m_cameraShakeTimer = 0.0f;
}

} // namespace mine
