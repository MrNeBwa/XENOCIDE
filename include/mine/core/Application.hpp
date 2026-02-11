#pragma once

struct GLFWwindow;

namespace mine {

class Renderer;

class Application {
public:
    Application(int width, int height, const char* title);
    ~Application();

    void run();

private:
    void processInput();
    void update(float deltaTime);
    void render();

private:
    GLFWwindow* m_window = nullptr;
    Renderer* m_renderer = nullptr;
    bool m_running = true;
    float m_lastFrameTime = 0.0f;
    float m_playerX = 400.0f;
    float m_playerY = 300.0f;
};

} // namespace mine
