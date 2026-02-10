#include "mine/core/Application.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

namespace mine {

Application::Application(int width, int height, const char* title) {
    if (!glfwInit()) {
        std::cerr << "❌ Failed to initialize GLFW\n";
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        std::cerr << "❌ Failed to create GLFW window\n";
        glfwTerminate();
        exit(1);
    }
    glfwMakeContextCurrent(m_window);
    
    std::cout << "✅ OpenGL window created: " << title << " (" << width << "x" << height << ")\n";
}

Application::~Application() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::processInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }
}

void Application::update(float deltaTime) {
    // TODO: обновление логики игры
    (void)deltaTime; // заглушка для предотвращения предупреждения
}

void Application::render() {
    // Очистка экрана цветом (космический фон: тёмно-синий)
    glClearColor(0.05f, 0.02f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Application::run() {
    std::cout << "🎮 Game loop started...\n";
    std::cout << "   Press ESC to exit\n\n";

    // Главный цикл
    while (!glfwWindowShouldClose(m_window) && m_running) {
        // Время для дельты
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - m_lastFrameTime;
        m_lastFrameTime = currentFrame;

        // Обработка ввода
        processInput();

        // Обновление логики
        update(deltaTime);

        // Рендеринг
        render();

        // Своп буферов и обработка событий
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    std::cout << "\n👋 Game closed cleanly\n";
}

} // namespace mine
