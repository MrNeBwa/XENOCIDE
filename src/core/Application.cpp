// ⚠️ GLAD ДОЛЖЕН БЫТЬ АБСОЛЮТНО ПЕРВЫМ — НИЧЕГО ДО ЭТОГО!
#include <glad/glad.h>
// ⚠️ GLFW — СТРОГО ВТОРЫМ, чтобы он увидел макросы GLAD и пропустил системные
// заголовки
#include <GLFW/glfw3.h>

// Теперь безопасно подключать остальное
#include "mine/core/Application.hpp"
#include "mine/graphics/Renderer.hpp"
#include "mine/math/Vector2.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
namespace mine {

Application::Application(int width, int height, const char *title)
    : m_lastFrameTime(static_cast<float>(glfwGetTime())), m_running(true),
      m_randomEngine(std::random_device{}()), m_randomDist(-1.0f, 1.0f) {
  if (!glfwInit()) {
    std::cerr << "❌ GLFW init failed\n";
    std::exit(1);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!m_window) {
    std::cerr << "❌ Window creation failed\n";
    glfwTerminate();
    std::exit(1);
  }
  glfwMakeContextCurrent(m_window);

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    std::cerr << "❌ GLAD init failed\n";
    std::exit(1);
  }

  std::cout << "✅ OpenGL " << glGetString(GL_VERSION) << "\n";

  m_renderer = new Renderer(width, height);
  if (!m_renderer) {
    std::cerr << "❌ Renderer allocation failed\n";
    glfwTerminate();
    std::exit(1);
  }

  // Start player at center of screen
  m_playerPos = {static_cast<float>(width) / 2.0f,
                 static_cast<float>(height) / 2.0f};
  m_cameraPos = m_playerPos;
  m_cameraTarget = m_playerPos;
}

Application::~Application() {
  delete m_renderer;
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

void Application::processInput(float deltaTime) {
  constexpr float speed = 300.0f;

  // 1. Управление выходом
  if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    m_running = false;
  }

  // 2. Тряска камеры (Space)
  if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    if (!m_hasShaken) {
      shakeCamera(10.0f, 0.3f);
      m_hasShaken = true;
    }
  } else {
    m_hasShaken = false;
  }

  // 3. Передвижение (WASD / Arrows)
  if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
    m_playerPos.x -= speed * deltaTime;
  if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
    m_playerPos.x += speed * deltaTime;
  if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
    m_playerPos.y += speed * deltaTime;
  if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
    m_playerPos.y -= speed * deltaTime;

  // Ограничение движения (Bounds)
  m_playerPos.x = std::clamp(m_playerPos.x, 32.0f, 2000.0f - 32.0f);
  m_playerPos.y = std::clamp(m_playerPos.y, 32.0f, 2000.0f - 32.0f);
  // 1. Получаем координаты мыши (0,0 — левый верх окна)
  double mouseX, mouseY;
  glfwGetCursorPos(m_window, &mouseX, &mouseY);

  int winWidth, winHeight;
  glfwGetWindowSize(m_window, &winWidth, &winHeight);

  // 2. ПЕРЕВОДИМ МЫШЬ В ВЕКТОР ОТНОСИТЕЛЬНО ЦЕНТРА ЭКРАНА
  // Мы не считаем мировые координаты через камеру (это часто плодит ошибки),
  // мы просто смотрим, куда направлен курсор относительно центра монитора.
  float screenDirX =
      static_cast<float>(mouseX) - (static_cast<float>(winWidth) * 0.5f);
  float screenDirY =
      (static_cast<float>(winHeight) * 0.5f) - static_cast<float>(mouseY);

  // 3. СЧИТАЕМ УГОЛ
  // ВАЖНО: atan2(y, x)
  // screenDirY уже инвертирован выше, так что положительный Y — это "вверх" по
  // экрану.
  float angle = std::atan2(screenDirY, screenDirX);

  // 4. КОРРЕКЦИЯ ПОД ВЕРТИКАЛЬНЫЙ СПРАЙТ
  // Если твой персонаж нарисован "лицом вверх", вычитаем 90 градусов (PI/2).
  m_playerRotation = angle - 1.57079632679f;
}

void Application::update(float deltaTime) {
  // Smooth camera follow (был пропущен в предыдущей версии!)
  m_cameraPos.x += (m_playerPos.x - m_cameraPos.x) * m_cameraSmooth;
  m_cameraPos.y += (m_playerPos.y - m_cameraPos.y) * m_cameraSmooth;

  // Camera shake decay with smooth fade-out
  if (m_cameraShakeDuration > 0.0f) {
    m_cameraShakeTimer += deltaTime;
    float progress = std::min(m_cameraShakeTimer / m_cameraShakeDuration, 1.0f);

    // Ease-out decay for natural feel
    m_cameraShakeIntensity *= (1.0f - progress);

    if (progress >= 1.0f) {
      m_cameraShakeDuration = 0.0f;
      m_cameraShakeIntensity = 0.0f;
      m_cameraShakeTimer = 0.0f;
    }
  }
}

void Application::render() {
  Vector2 shakeOffset{0.0f, 0.0f};
  if (m_cameraShakeIntensity > 0.0f) {
    // Proper random shake using seeded engine
    shakeOffset.x = m_randomDist(m_randomEngine) * m_cameraShakeIntensity;
    shakeOffset.y = m_randomDist(m_randomEngine) * m_cameraShakeIntensity;
  }

  m_renderer->beginFrame();
  m_renderer->setCameraPosition(m_cameraPos, shakeOffset);

  // Draw scene
  m_renderer->drawQuad({0, 0}, {2000, 2000}, 0.3f, 0.3f, 0.3f);   // Background
  m_renderer->drawQuad({200, 200}, {100, 100}, 0.2f, 0.6f, 0.8f); // Blue box
  m_renderer->drawQuad({0, 0}, {150, 80}, 0.7f, 0.3f, 0.1f);      // Orange box
  m_renderer->drawQuad(m_playerPos, {64, 64}, 0.9f, 0.2f, 0.8f,
                       m_playerRotation); // Player

  m_renderer->endFrame();
}

void Application::run() {
  std::cout << "\n🎮 XENOCIDE running! Move with WASD/arrow keys. Mouse to "
               "aim. ESC to exit.\n";
  std::cout << "   Press SPACE to shake camera\n\n";

  while (!glfwWindowShouldClose(m_window) && m_running) {
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;

    processInput(deltaTime);
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
