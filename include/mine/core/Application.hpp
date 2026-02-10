#pragma once

struct GLFWwindow;

namespace mine {

class Application {
public:
    Application(int width, int height, const char* title);
    ~Application();

    void run();  // Полноценный игровой цикл

private:
    void processInput();
    void update(float deltaTime);
    void render();

private:
    GLFWwindow* m_window = nullptr;
    bool m_running = true;
    float m_lastFrameTime = 0.0f;
};

} // namespace mine
