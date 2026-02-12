#include <glad/glad.h>
#include "mine/core/Application.hpp"
#include "mine/graphics/Renderer.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <cstdlib>

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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "❌ GLAD init failed\n";
        exit(1);
    }

    std::cout << "✅ OpenGL " << glGetString(GL_VERSION) << "\n";
    m_renderer = new Renderer(width, height);
    
    // Стартуем с центра экрана
    m_cameraPos = {width / 2.0f, height / 2.0f};
    m_cameraTarget = m_cameraPos;
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
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        shakeCamera(10.0f, 0.3f); // Интенсивность 10, длительность 0.3 сек
    }
    // Движение игрока
    if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_playerPos.x -= speed * deltaTime;
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_playerPos.x += speed * deltaTime;
    if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_playerPos.y -= speed * deltaTime;
    if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_KEY_S)
        m_playerPos.y += speed * deltaTime;

    // Поворот игрока к курсору мыши
    double mouseX, mouseY;
    int winWidth, winHeight;
    glfwGetCursorPos(m_window, &mouseX, &mouseY);
    glfwGetWindowSize(m_window, &winWidth, &winHeight);

    float screenMouseX = (float)mouseX;
    float screenMouseY = (float)(winHeight - mouseY);

    float dx = screenMouseX - m_playerPos.x;
    float dy = screenMouseY - m_playerPos.y;
    m_playerRotation = atan2(dy, dx);
}

void Application::update(float deltaTime) {
    // Сглаживаем позицию камеры к позиции игрока
    m_cameraTarget = m_playerPos;
    m_cameraPos.x += (m_cameraTarget.x - m_cameraPos.x) * m_cameraSmooth;
    m_cameraPos.y += (m_cameraTarget.y - m_cameraPos.y) * m_cameraSmooth;

    // Обработка тряски камеры
    if (m_cameraShakeDuration > 0.0f) {
        m_cameraShakeTimer += deltaTime;
        float progress = m_cameraShakeTimer / m_cameraShakeDuration;
        m_cameraShakeIntensity = (1.0f - progress) * m_cameraShakeIntensity;
        
        if (progress >= 1.0f) {
            m_cameraShakeDuration = 0.0f;
            m_cameraShakeIntensity = 0.0f;
            m_cameraShakeTimer = 0.0f;
        }
    }
}

void Application::render() {
    // Генерируем случайные смещения для тряски камеры
    float shakeX = 0.0f;
    float shakeY = 0.0f;
    if (m_cameraShakeIntensity > 0.0f) {
        shakeX = (rand() % 100 - 50) / 100.0f * m_cameraShakeIntensity;
        shakeY = (rand() % 100 - 50) / 100.0f * m_cameraShakeIntensity;
    }

    m_renderer->beginFrame();
    
    // Устанавливаем камеру с учётом тряски
    m_renderer->setCameraPosition(m_cameraPos, {shakeX, shakeY});
    
    // Карта корабля (большой серый квадрат)
    m_renderer->drawQuad({0, 0}, {2000, 2000}, 0.3f, 0.3f, 0.3f); // Теперь карта больше экрана

    // Декорации корабля пришельцев
    m_renderer->drawQuad({200, 200}, {100, 100}, 0.2f, 0.6f, 0.8f); // Синий
    m_renderer->drawQuad({600, 400}, {150, 80},  0.7f, 0.3f, 0.1f); // Оранжевый

    // Игрок (розовый квадрат)
    m_renderer->drawQuad(m_playerPos, {64, 64}, 0.9f, 0.2f, 0.8f, m_playerRotation);

    m_renderer->endFrame();
}

void Application::run() {
    std::cout << "\n🎮 XENOCIDE running! Move with WASD/arrow keys. Mouse to aim. ESC to exit.\n";
    std::cout << "   Press SPACE to shake camera (like in Hotline Miami)\n\n";

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

// Новая функция: тряска камеры
void Application::shakeCamera(float intensity, float duration) {
    m_cameraShakeIntensity = intensity;
    m_cameraShakeDuration = duration;
    m_cameraShakeTimer = 0.0f;
}

} // namespace mine
