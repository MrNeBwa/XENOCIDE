// ⚠️ GLAD ДОЛЖЕН БЫТЬ АБСОЛЮТНО ПЕРВЫМ — НИЧЕГО ДО ЭТОГО!
#include <glad/glad.h>
// ⚠️ GLFW — СТРОГО ВТОРЫМ
#include <GLFW/glfw3.h>

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
#include <vector>

namespace mine {

// Вспомогательная функция для расширения пути ~
static std::string expandPath(const std::string &path) {
  if (!path.empty() && path[0] == '~') {
    const char *home = std::getenv("HOME");
    if (!home)
      home = std::getenv("USERPROFILE");
    if (home)
      return std::string(home) + path.substr(1);
  }
  return path;
}

Application::Application(int width, int height, const char *title)
    : m_lastFrameTime(static_cast<float>(glfwGetTime())), m_running(true),
      m_randomEngine(std::random_device{}()), m_randomDist(-1.0f, 1.0f),
      mWindowWidth(width), mWindowHeight(height) {

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

  // Загрузка текстуры игрока
  m_playerTexture = std::make_shared<Texture>(
      expandPath("~/Projects/XENOCIDE/assets/textures/player.png").c_str());
  if (!m_playerTexture)
    std::cerr << "⚠️  Texture load failed, using fallback\n";

  m_playerPos = {static_cast<float>(width) / 2.0f,
                 static_cast<float>(height) / 2.0f};
  m_cameraPos = m_playerPos;

  generateFloors();
}

Application::~Application() {
  delete m_renderer;
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

void Application::generateFloors() {
  // Clear any existing data and use consistent world size
  m_gameObjects.clear();
  m_elevators.clear();
  m_stairs.clear();

  const float worldSize = m_worldSize;
  const float padding = 40.0f;
  const float elevatorSize = 80.0f;
  const float stairSize = 100.0f;

  // === УНИКАЛЬНЫЕ ДАННЫЕ ДЛЯ КАЖДОГО ЭТАЖА ===
  // Формат: {x, y, width, height, isSolid, texturePath}
  // isSolid=true = стена (коллизия), false = декор/комната

  struct FloorLayout {
    float bgColor[3];
    std::vector<std::vector<float>> objects; // {x, y, w, h, isSolid}
  };

  std::vector<FloorLayout> floors = {
      // ЭТАЖ 0: B1 (Basement) - Тёмный, много стен
      {{0.15f, 0.10f, 0.25f},
       {
           // Стены по периметру
           {100, 100, 1800, 50, 1.0f},
           {100, 1850, 1800, 50, 1.0f},
           {100, 100, 50, 1800, 1.0f},
           {1850, 100, 50, 1800, 1.0f},
           // Внутренние стены (лабиринт)
           {400, 400, 600, 50, 1.0f},
           {1000, 600, 50, 400, 1.0f},
           {600, 1000, 500, 50, 1.0f},
           {1400, 400, 50, 600, 1.0f},
           // Комнаты (не сплошные)
           {300, 300, 200, 200, 0.0f},
           {1500, 1500, 200, 200, 0.0f},
       }},
      // ЭТАЖ 1: G (Ground) - Открытый, мало стен
      {{0.20f, 0.25f, 0.15f},
       {
           {100, 100, 1800, 50, 1.0f},
           {100, 1850, 1800, 50, 1.0f},
           {100, 100, 50, 1800, 1.0f},
           {1850, 100, 50, 1800, 1.0f},
           // Немного внутренних стен
           {500, 500, 400, 50, 1.0f},
           {1100, 1000, 400, 50, 1.0f},
           // Большие комнаты
           {300, 300, 300, 300, 0.0f},
           {1400, 300, 300, 300, 0.0f},
           {300, 1400, 300, 300, 0.0f},
           {1400, 1400, 300, 300, 0.0f},
       }},
      // ЭТАЖ 2: F1 (First) - Сложная структура
      {{0.25f, 0.15f, 0.15f},
       {
           {100, 100, 1800, 50, 1.0f},
           {100, 1850, 1800, 50, 1.0f},
           {100, 100, 50, 1800, 1.0f},
           {1850, 100, 50, 1800, 1.0f},
           // Крестообразные стены
           {900, 100, 200, 800, 1.0f},
           {900, 1100, 200, 800, 1.0f},
           {100, 900, 800, 200, 1.0f},
           {1100, 900, 800, 200, 1.0f},
           // Комнаты по секторам
           {200, 200, 400, 400, 0.0f},
           {1400, 200, 400, 400, 0.0f},
           {200, 1400, 400, 400, 0.0f},
           {1400, 1400, 400, 400, 0.0f},
       }}};

  // Генерация объектов для каждого этажа
  for (int floor = 0; floor < 3; ++floor) {
    for (const auto &obj : floors[floor].objects) {
      GameObject go;
      go.position = {obj[0] + obj[2] / 2, obj[1] + obj[3] / 2};
      go.size = {obj[2], obj[3]};
      go.floorLevel = floor;
      go.isSolid = (obj[4] > 0.5f);
      go.r = go.isSolid ? 0.5f : 0.4f + (floor * 0.1f);
      go.g = go.isSolid ? 0.5f : 0.3f + (floor * 0.1f);
      go.b = go.isSolid ? 0.6f : 0.5f - (floor * 0.1f);
      go.texture = nullptr; // Можно добавить загрузку текстур здесь
      m_gameObjects.push_back(go);
    }

    // Лифты (2 угла: левый-низ и правый-верх)
    // Place elevators a bit inside the padding so they are reachable inside walls
    m_elevators.push_back({{padding + elevatorSize, padding + elevatorSize},
                 floor,
                 true});
    m_elevators.push_back({{worldSize - padding - elevatorSize,
                worldSize - padding - elevatorSize},
                 floor,
                 false});

    // Лестницы (2 угла: правый-низ и левый-верх)
    m_stairs.push_back(
      {{worldSize - padding - stairSize, padding + stairSize}, floor, false});
    m_stairs.push_back(
      {{padding + stairSize, worldSize - padding - stairSize}, floor, true});
  }

  std::cout << "✅ Generated 3 unique floors with " << m_gameObjects.size()
            << " objects, " << m_elevators.size() << " elevators, "
            << m_stairs.size() << " stairs\n";
}

bool Application::canUseElevator(const Vector2 &playerPos,
                                 Elevator **outElevator) {
  const float interactionRadius = 120.0f;
  for (auto &elevator : m_elevators) {
    if (elevator.floorLevel == mCurrentFloor) {
      float dx = playerPos.x - elevator.position.x;
      float dy = playerPos.y - elevator.position.y;
      if (dx * dx + dy * dy < interactionRadius * interactionRadius) {
        if (outElevator)
          *outElevator = &elevator;
        return true;
      }
    }
  }
  return false;
}

bool Application::canUseStair(const Vector2 &playerPos, Stair **outStair) {
  const float interactionRadius = 120.0f;
  for (auto &stair : m_stairs) {
    if (stair.floorLevel == mCurrentFloor) {
      float dx = playerPos.x - stair.position.x;
      float dy = playerPos.y - stair.position.y;
      if (dx * dx + dy * dy < interactionRadius * interactionRadius) {
        if (outStair)
          *outStair = &stair;
        return true;
      }
    }
  }
  return false;
}

bool Application::checkWallCollision(const Vector2 &newPos, float radius) {
  for (const auto &obj : m_gameObjects) {
    if (obj.isSolid && obj.floorLevel == mCurrentFloor) {
      // AABB vs Circle collision
      float closestX = std::clamp(newPos.x, obj.position.x - obj.size.x / 2,
                                  obj.position.x + obj.size.x / 2);
      float closestY = std::clamp(newPos.y, obj.position.y - obj.size.y / 2,
                                  obj.position.y + obj.size.y / 2);
      float dx = newPos.x - closestX;
      float dy = newPos.y - closestY;
      if (dx * dx + dy * dy < radius * radius)
        return true; // Collision!
    }
  }
  return false;
}

void Application::processInput(float deltaTime) {
  constexpr float speed = 300.0f;
  Vector2 desiredPos = m_playerPos;

  // 1. Выход
  if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    m_running = false;

  // 2. Тряска камеры
  if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    if (!m_hasShaken) {
      shakeCamera(10.0f, 0.3f);
      m_hasShaken = true;
    }
  } else {
    m_hasShaken = false;
  }

  // 3. Движение (с коллизией)
  if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
    desiredPos.x -= speed * deltaTime;
  if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
    desiredPos.x += speed * deltaTime;
  if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
    desiredPos.y += speed * deltaTime;
  if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
    desiredPos.y -= speed * deltaTime;

  // Проверка коллизий по осям отдельно (для скольжения вдоль стен)
  if (!checkWallCollision({desiredPos.x, m_playerPos.y}, 30.0f))
    m_playerPos.x = desiredPos.x;
  if (!checkWallCollision({m_playerPos.x, desiredPos.y}, 30.0f))
    m_playerPos.y = desiredPos.y;

  // Ограничение мира
  m_playerPos.x = std::clamp(m_playerPos.x, 32.0f, 2000.0f - 32.0f);
  m_playerPos.y = std::clamp(m_playerPos.y, 32.0f, 2000.0f - 32.0f);

  // 4. Лифт - выбор этажа (клавиши 1, 2, 3)
  Elevator *elevator = nullptr;
  mNearElevator = canUseElevator(m_playerPos, &elevator);

  if (mNearElevator && !mElevatorCooldown) {
    if (glfwGetKey(m_window, GLFW_KEY_1) == GLFW_PRESS) {
      mSelectedFloor = 0;
      mElevatorCooldown = true;
    } else if (glfwGetKey(m_window, GLFW_KEY_2) == GLFW_PRESS) {
      mSelectedFloor = 1;
      mElevatorCooldown = true;
    } else if (glfwGetKey(m_window, GLFW_KEY_3) == GLFW_PRESS) {
      mSelectedFloor = 2;
      mElevatorCooldown = true;
    }

    if (mElevatorCooldown && mCurrentFloor != mSelectedFloor) {
      mCurrentFloor = mSelectedFloor;
      std::cout << "🛗 Teleported to floor " << (mCurrentFloor + 1) << "\n";
    }
  }
  if (glfwGetKey(m_window, GLFW_KEY_1) == GLFW_RELEASE &&
      glfwGetKey(m_window, GLFW_KEY_2) == GLFW_RELEASE &&
      glfwGetKey(m_window, GLFW_KEY_3) == GLFW_RELEASE) {
    mElevatorCooldown = false;
  }

  // 5. Лестница - переход на 1 этаж вверх/вниз (E / Q)
  Stair *stair = nullptr;
  if (canUseStair(m_playerPos, &stair)) {
    if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS && !mElevatorCooldown) {
      if (mCurrentFloor < 2) {
        mCurrentFloor++;
        mElevatorCooldown = true;
        std::cout << "🪜 Went UP\n";
      }
    }
    if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS && !mElevatorCooldown) {
      if (mCurrentFloor > 0) {
        mCurrentFloor--;
        mElevatorCooldown = true;
        std::cout << "🪜 Went DOWN\n";
      }
    }
    if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_RELEASE &&
        glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_RELEASE)
      mElevatorCooldown = false;
  }

  // 6. Вращение к мыши
  int winWidth, winHeight;
  glfwGetWindowSize(m_window, &winWidth, &winHeight);
  double mouseX, mouseY;
  glfwGetCursorPos(m_window, &mouseX, &mouseY);
  float screenMouseX = static_cast<float>(mouseX);
  float screenMouseY =
      static_cast<float>(winHeight) - static_cast<float>(mouseY);
  float screenPlayerX = (m_playerPos.x - m_cameraPos.x) + (winWidth * 0.5f);
  float screenPlayerY = (m_playerPos.y - m_cameraPos.y) + (winHeight * 0.5f);
  float angle =
      std::atan2(screenMouseY - screenPlayerY, screenMouseX - screenPlayerX);
  // Sprite in art points upwards; rotate so it faces the mouse.
  const float PI = 3.14159265358979323846f;
  m_playerRotation = angle + (PI * 0.5f);
}

void Application::update(float deltaTime) {
  m_cameraPos.x += (m_playerPos.x - m_cameraPos.x) * m_cameraSmooth;
  m_cameraPos.y += (m_playerPos.y - m_cameraPos.y) * m_cameraSmooth;

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

  const float floorColors[3][3] = {
      {0.15f, 0.10f, 0.25f}, {0.20f, 0.25f, 0.15f}, {0.25f, 0.15f, 0.15f}};

  // Фон этажа
  // Draw floor background using the configured world size
  m_renderer->drawQuad({m_worldSize / 2.0f, m_worldSize / 2.0f}, {m_worldSize,
                                                           m_worldSize},
                        floorColors[mCurrentFloor][0],
                        floorColors[mCurrentFloor][1],
                        floorColors[mCurrentFloor][2], 0.0f);

  // Объекты (стены и комнаты)
  for (const auto &obj : m_gameObjects) {
    if (obj.floorLevel == mCurrentFloor) {
      if (obj.texture) {
        m_renderer->drawQuad(obj.position, obj.size, obj.texture.get(), 0.0f);
      } else {
        m_renderer->drawQuad(obj.position, obj.size, obj.r, obj.g, obj.b, 0.0f);
      }
    }
  }

  // Лифты
  for (const auto &elevator : m_elevators) {
    if (elevator.floorLevel == mCurrentFloor) {
      float dx = m_playerPos.x - elevator.position.x;
      float dy = m_playerPos.y - elevator.position.y;
      bool nearby = (dx * dx + dy * dy < 150.0f * 150.0f);
      m_renderer->drawQuad(elevator.position, {80, 80}, nearby ? 0.9f : 0.5f,
                           nearby ? 0.3f : 0.3f, nearby ? 0.4f : 0.7f, 0.0f);
    }
  }

  // Лестницы
  for (const auto &stair : m_stairs) {
    if (stair.floorLevel == mCurrentFloor) {
      float dx = m_playerPos.x - stair.position.x;
      float dy = m_playerPos.y - stair.position.y;
      bool nearby = (dx * dx + dy * dy < 150.0f * 150.0f);
      // Рисуем ступеньки (3 полосы)
      for (int i = 0; i < 3; ++i) {
        m_renderer->drawQuad(
            {stair.position.x, stair.position.y - 20.0f + i * 15.0f}, {100, 8},
            nearby ? 0.8f : 0.6f, nearby ? 0.6f : 0.4f, nearby ? 0.2f : 0.3f,
            0.0f);
      }
    }
  }

  // Игрок
  if (m_playerTexture) {
    m_renderer->drawQuad(m_playerPos, {64, 64}, m_playerTexture.get(),
                         m_playerRotation);
  } else {
    m_renderer->drawQuad(m_playerPos, {64, 64}, 0.9f, 0.2f, 0.8f,
                         m_playerRotation);
  }

  // UI: Панель лифта (если рядом)
  if (mNearElevator) {
    float uiX = m_playerPos.x - m_cameraPos.x + mWindowWidth * 0.5f;
    float uiY = m_playerPos.y - m_cameraPos.y + mWindowHeight * 0.5f + 80.0f;

    // Фон панели
    m_renderer->drawQuad({uiX, uiY}, {200, 120}, 0.1f, 0.1f, 0.15f, 0.0f);
    m_renderer->drawQuad({uiX, uiY}, {196, 116}, 0.2f, 0.2f, 0.25f, 0.0f);

    // Кнопки этажей
    for (int i = 0; i < 3; ++i) {
      float btnY = uiY - 30.0f + i * 35.0f;
      bool selected = (mSelectedFloor == i);
      m_renderer->drawQuad({uiX, btnY}, {150, 25}, selected ? 0.9f : 0.3f,
                           selected ? 0.3f : 0.3f, selected ? 0.4f : 0.3f,
                           0.0f);
      // Номер этажа (полоска)
      m_renderer->drawQuad({uiX - 50.0f, btnY}, {30, 15}, 1.0f, 1.0f, 1.0f,
                           0.0f);
    }

    // Подсказка
    m_renderer->drawQuad({uiX, uiY + 50.0f}, {180, 15}, 0.7f, 0.7f, 0.7f, 0.0f);
  }

  // UI: Индикатор этажа (левый верхний угол)
  {
    float uiX = 50.0f;
    float uiY = mWindowHeight - 50.0f;
    m_renderer->drawQuad({uiX, uiY}, {220, 40}, 0.05f, 0.05f, 0.1f, 0.0f);
    for (int i = 0; i < 3; ++i) {
      float labelX = uiX + i * 70.0f + 20.0f;
      if (i == mCurrentFloor) {
        m_renderer->drawQuad({labelX, uiY}, {50, 30}, floorColors[i][0] * 1.8f,
                             floorColors[i][1] * 1.8f, floorColors[i][2] * 1.8f,
                             0.0f);
      }
      m_renderer->drawQuad({labelX - 15.0f, uiY}, {30, 20}, 1.0f, 1.0f, 1.0f,
                           0.0f);
    }
  }

  m_renderer->endFrame();
}

void Application::run() {
  std::cout << "\n🎮 XENOCIDE running!\n";
  std::cout << "   WASD — move | Mouse — aim | SPACE — shake\n";
  std::cout
      << "   1/2/3 — elevator floors (when near) | E/Q — stairs up/down\n";
  std::cout << "   ESC — exit\n\n";

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
