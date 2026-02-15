// ⚠️ GLAD ДОЛЖЕН БЫТЬ АБСОЛЮТНО ПЕРВЫМ — НИЧЕГО ДО ЭТОГО!
#include <glad/glad.h>
// ⚠️ GLFW — СТРОГО ВТОРЫМ, чтобы он увидел макросы GLAD и пропустил системные
// заголовки
#include <GLFW/glfw3.h>

// Теперь безопасно подключать остальное
#include "mine/core/Application.hpp"
#include "mine/graphics/Renderer.hpp"
#include "mine/graphics/Texture.hpp"
#include "mine/math/Vector2.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector> // Для хранения этажей

namespace mine {

// Локальные структуры для системы этажей (не требуют изменений в
// Application.hpp)
struct Room {
  Vector2 position;
  Vector2 size;
  int floorLevel;
};

struct Elevator {
  Vector2 position;
  int floorLevel;
};

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

  // ✅ Расширяем ~ до реального пути домашней директории
  std::string texturePath = "~/Projects/XENOCIDE/assets/textures/player.png";
  if (!texturePath.empty() && texturePath[0] == '~') {
    const char *home = std::getenv("HOME"); // Unix-like (Linux/macOS)
    if (!home) {
      home = std::getenv("USERPROFILE"); // Windows
    }
    if (home) {
      texturePath = std::string(home) + texturePath.substr(1);
    } else {
      std::cerr
          << "⚠️  Warning: Could not expand '~' (HOME/USERPROFILE not set). "
          << "Using raw path.\n";
    }
  }

  m_playerTexture = std::make_shared<Texture>(texturePath.c_str());
  if (!m_playerTexture) {
    std::cerr << "⚠️  Texture allocation failed. Using colored fallback.\n";
  }

  // Сохраняем размеры окна для UI
  m_windowWidth = width;
  m_windowHeight = height;

  // Start player at center of screen
  m_playerPos = {static_cast<float>(width) / 2.0f,
                 static_cast<float>(height) / 2.0f};
  m_cameraPos = m_playerPos;
  m_cameraTarget = m_playerPos;

  // Генерация этажей
  generateFloors();
}

Application::~Application() {
  delete m_renderer;
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

void Application::generateFloors() {
  const float worldSize = 2000.0f;
  const float roomSize = 200.0f;
  const float corridorWidth = 120.0f;
  const float padding = 40.0f;

  // Generate 3 floors
  for (int floor = 0; floor < 3; ++floor) {
    // Rooms along top edge
    for (float x = padding; x <= worldSize - roomSize - padding;
         x += roomSize + corridorWidth) {
      m_rooms.push_back({{x + roomSize / 2, worldSize - padding - roomSize / 2},
                         {roomSize, roomSize},
                         floor});
    }
    // Rooms along bottom edge
    for (float x = padding; x <= worldSize - roomSize - padding;
         x += roomSize + corridorWidth) {
      m_rooms.push_back({{x + roomSize / 2, padding + roomSize / 2},
                         {roomSize, roomSize},
                         floor});
    }
    // Rooms along left edge (skip corners)
    for (float y = padding + roomSize + corridorWidth;
         y <= worldSize - roomSize - padding - roomSize - corridorWidth;
         y += roomSize + corridorWidth) {
      m_rooms.push_back({{padding + roomSize / 2, y + roomSize / 2},
                         {roomSize, roomSize},
                         floor});
    }
    // Rooms along right edge (skip corners)
    for (float y = padding + roomSize + corridorWidth;
         y <= worldSize - roomSize - padding - roomSize - corridorWidth;
         y += roomSize + corridorWidth) {
      m_rooms.push_back({{worldSize - padding - roomSize / 2, y + roomSize / 2},
                         {roomSize, roomSize},
                         floor});
    }

    // Elevators at 4 corners
    float elevatorSize = 80.0f;
    m_elevators.push_back(
        {{padding + elevatorSize / 2, padding + elevatorSize / 2},
         floor}); // Bottom-left
    m_elevators.push_back(
        {{worldSize - padding - elevatorSize / 2, padding + elevatorSize / 2},
         floor}); // Bottom-right
    m_elevators.push_back(
        {{padding + elevatorSize / 2, worldSize - padding - elevatorSize / 2},
         floor}); // Top-left
    m_elevators.push_back({{worldSize - padding - elevatorSize / 2,
                            worldSize - padding - elevatorSize / 2},
                           floor}); // Top-right
  }

  std::cout << "✅ Generated 3 floors with " << m_rooms.size() << " rooms and "
            << m_elevators.size() << " elevator positions\n";
}

bool Application::canUseElevator(const Vector2 &playerPos) {
  const float interactionRadius = 100.0f;

  for (const auto &elevator : m_elevators) {
    if (elevator.floorLevel == m_currentFloor) {
      float dx = playerPos.x - elevator.position.x;
      float dy = playerPos.y - elevator.position.y;
      if (dx * dx + dy * dy < interactionRadius * interactionRadius) {
        return true;
      }
    }
  }
  return false;
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

  // 3. Переключение этажа через лифт (клавиша E)
  if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS && !m_elevatorCooldown) {
    if (canUseElevator(m_playerPos)) {
      m_currentFloor = (m_currentFloor + 1) % 3; // Cycle: 0 → 1 → 2 → 0
      m_elevatorCooldown = true;

      std::cout << "🛗 Switched to floor ";
      switch (m_currentFloor) {
      case 0:
        std::cout << "B1 (Basement)";
        break;
      case 1:
        std::cout << "G (Ground)";
        break;
      case 2:
        std::cout << "F1 (First)";
        break;
      }
      std::cout << "\n";
    }
  } else if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_RELEASE) {
    m_elevatorCooldown = false;
  }

  // 4. Передвижение (WASD / Arrows)
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

  // 5. Вращение спрайта к мыши
  int winWidth, winHeight;
  glfwGetWindowSize(m_window, &winWidth, &winHeight);

  double mouseX, mouseY;
  glfwGetCursorPos(m_window, &mouseX, &mouseY);

  // Нормализуем координаты мыши (0,0 — левый нижний угол)
  float screenMouseX = static_cast<float>(mouseX);
  float screenMouseY =
      static_cast<float>(winHeight) - static_cast<float>(mouseY);

  // Позиция игрока на экране относительно камеры
  float screenPlayerX =
      (m_playerPos.x - m_cameraPos.x) + (static_cast<float>(winWidth) * 0.5f);
  float screenPlayerY =
      (m_playerPos.y - m_cameraPos.y) + (static_cast<float>(winHeight) * 0.5f);

  // Вектор от игрока к мыши
  float dx = screenMouseX - screenPlayerX;
  float dy = screenMouseY - screenPlayerY;

  // Угол в радианах (0 = право)
  float angle = std::atan2(dy, dx);

  // Коррекция для спрайта, направленного вверх (вычитаем 90°)
  m_playerRotation = angle - 1.57079632679f; // -π/2
}

void Application::update(float deltaTime) {
  // Smooth camera follow
  m_cameraPos.x += (m_playerPos.x - m_cameraPos.x) * m_cameraSmooth;
  m_cameraPos.y += (m_playerPos.y - m_cameraPos.y) * m_cameraSmooth;

  // Camera shake decay
  if (m_cameraShakeDuration > 0.0f) {
    m_cameraShakeTimer += deltaTime;
    float progress = std::min(m_cameraShakeTimer / m_cameraShakeDuration, 1.0f);
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
    shakeOffset.x = m_randomDist(m_randomEngine) * m_cameraShakeIntensity;
    shakeOffset.y = m_randomDist(m_randomEngine) * m_cameraShakeIntensity;
  }

  m_renderer->beginFrame();
  m_renderer->setCameraPosition(m_cameraPos, shakeOffset);

  // Floor background colors (darker for lower floors)
  const float floorColors[3][3] = {
      {0.15f, 0.10f, 0.25f}, // B1: Dark purple
      {0.20f, 0.25f, 0.15f}, // G:  Greenish
      {0.25f, 0.15f, 0.15f}  // F1: Reddish
  };

  // Draw current floor background
  m_renderer->drawQuad(
      {1000, 1000}, {2000, 2000}, floorColors[m_currentFloor][0],
      floorColors[m_currentFloor][1], floorColors[m_currentFloor][2]);

  // Draw rooms on current floor
  for (const auto &room : m_rooms) {
    if (room.floorLevel == m_currentFloor) {
      float r = 0.4f + (static_cast<int>(room.position.x) % 2) * 0.15f;
      float g = 0.3f + (static_cast<int>(room.position.y) % 2) * 0.15f;
      float b = 0.5f - (room.floorLevel * 0.1f);
      m_renderer->drawQuad(room.position, room.size, r, g, b);
    }
  }

  // Draw elevators on current floor
  const float elevatorSize = 80.0f;
  for (const auto &elevator : m_elevators) {
    if (elevator.floorLevel == m_currentFloor) {
      float dx = m_playerPos.x - elevator.position.x;
      float dy = m_playerPos.y - elevator.position.y;
      bool nearby = (dx * dx + dy * dy < 150.0f * 150.0f);

      float r = nearby ? 0.9f : 0.5f;
      float g = nearby ? 0.3f : 0.3f;
      float b = nearby ? 0.4f : 0.7f;

      m_renderer->drawQuad(elevator.position, {elevatorSize, elevatorSize}, r,
                           g, b);

      // "E" indicator when nearby
      if (nearby) {
        // Horizontal bar of "E"
        m_renderer->drawQuad(
            {elevator.position.x - 15.0f, elevator.position.y + 25.0f}, {30, 8},
            1.0f, 1.0f, 1.0f);
        // Vertical bar of "E"
        m_renderer->drawQuad(
            {elevator.position.x - 25.0f, elevator.position.y + 5.0f}, {8, 40},
            1.0f, 1.0f, 1.0f);
        // Middle bar of "E"
        m_renderer->drawQuad(
            {elevator.position.x - 15.0f, elevator.position.y + 5.0f}, {20, 8},
            1.0f, 1.0f, 1.0f);
      }
    }
  }

  // Draw player (with fallback if texture loading failed)
  if (m_playerTexture) {
    m_renderer->drawQuad(m_playerPos, {64, 64}, m_playerTexture.get(),
                         m_playerRotation);
  } else {
    // Fallback colored quad
    m_renderer->drawQuad(m_playerPos, {64, 64}, 0.9f, 0.2f, 0.8f,
                         m_playerRotation);
  }

  // Floor indicator UI (top-left)
  {
    float uiX = 50.0f;
    float uiY = m_windowHeight - 50.0f;
    const char *floorNames[3] = {"B1", "G", "F1"};

    // Background bar
    m_renderer->drawQuad({uiX, uiY}, {220, 40}, 0.05f, 0.05f, 0.1f);

    // Floor labels
    for (int i = 0; i < 3; ++i) {
      float labelX = uiX + i * 70.0f + 20.0f;
      float labelY = uiY;

      // Highlight current floor
      if (i == m_currentFloor) {
        m_renderer->drawQuad({labelX, labelY}, {50, 30},
                             floorColors[i][0] * 1.8f, floorColors[i][1] * 1.8f,
                             floorColors[i][2] * 1.8f);
      }

      // Floor name text (simplified as colored bars)
      m_renderer->drawQuad({labelX - 15.0f, labelY + 5.0f}, {30, 8}, 1.0f, 1.0f,
                           1.0f); // Top bar
      m_renderer->drawQuad({labelX - 15.0f, labelY - 5.0f}, {30, 8}, 1.0f, 1.0f,
                           1.0f); // Bottom bar
      m_renderer->drawQuad({labelX - 15.0f, labelY}, {8, 18}, 1.0f, 1.0f,
                           1.0f); // Vertical bar
    }
  }

  m_renderer->endFrame();
}

void Application::run() {
  std::cout << "\n🎮 XENOCIDE running!\n";
  std::cout << "   WASD/Arrows — move | Mouse — aim | SPACE — shake camera\n";
  std::cout << "   E — use elevator (when near corner) | ESC — exit\n\n";

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
