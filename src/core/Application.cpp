#include "mine/core/Application.hpp"
#include "mine/graphics/Renderer.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

namespace mine {

Application::Application(int width, int height, const char* title) {
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

    // GLAD НЕ НУЖЕН для базового 2D рендеринга!
    // Функции OpenGL доступны напрямую через драйвер

    std::cout << "✅ OpenGL " << glGetString(GL_VERSION) << "\n";
    m_renderer = new Renderer(width, height);
}

Application::~Application() {
    delete m_renderer;
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::processInput() {
    float speed = 300.0f;
    float deltaTime = glfwGetTime() - m_lastFrameTime;

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, true);

    if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_playerX -= speed * deltaTime;
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_playerX += speed * deltaTime;
    if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_playerY -= speed * deltaTime;
    if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_KEY_S)
        m_playerY += speed * deltaTime;
}

void Application::update(float) {}


void Application::render() {
    m_renderer->beginFrame();
    
    // Декорации корабля пришельцев
    m_renderer->drawColoredQuad({200, 200}, {100, 100}, 0.2f, 0.6f, 0.8f); // синий
    m_renderer->drawColoredQuad({600, 400}, {150, 80},  0.7f, 0.3f, 0.1f); // оранжевый
    
    // Игрок
    m_renderer->drawColoredQuad({m_playerX, m_playerY}, {64, 64}, 0.9f, 0.2f, 0.8f); // розовый

    m_renderer->endFrame();
}

void Application::run() {
    std::cout << "🎮 XENOCIDE running! Move with WASD/arrow keys. ESC to exit.\n";

    while (!glfwWindowShouldClose(m_window) && m_running) {
        float current = (float)glfwGetTime();
        float dt = current - m_lastFrameTime;
        m_lastFrameTime = current;

        processInput();
        update(dt);
        render();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    std::cout << "\n👋 Exited cleanly\n";
}

} // namespace mine
