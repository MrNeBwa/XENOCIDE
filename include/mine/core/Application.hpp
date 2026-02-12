#pragma once

#include "mine/math/Vector2.hpp"

struct GLFWwindow;

namespace mine {

class Renderer;

class Application {
public:
    Application(int width, int height, const char* title);
    ~Application();

    void run();
    void shakeCamera(float intensity, float duration); // Новая функция

private:
    void processInput();
    void update(float deltaTime);
    void render();

    GLFWwindow* m_window = nullptr;
    Renderer* m_renderer = nullptr;
    bool m_running = true;
    float m_lastFrameTime = 0.0f;
    
    Vector2 m_playerPos = {400.0f, 300.0f};
    float m_playerRotation = 0.0f;
    
    // Камера
    Vector2 m_cameraPos = {0.0f, 0.0f};
    Vector2 m_cameraTarget = {0.0f, 0.0f};
    float m_cameraSmooth = 0.1f;
    
    // Тряска камеры
    float m_cameraShakeIntensity = 0.0f;
    float m_cameraShakeDuration = 0.0f;
    float m_cameraShakeTimer = 0.0f;
};

} // namespace mine
