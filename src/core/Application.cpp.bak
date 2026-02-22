// ⚠️ GLAD ДОЛЖЕН БЫТЬ АБСОЛЮТНО ПЕРВЫМ — НИЧЕГО ДО ЭТОГО!
#include <glad/glad.h>
// ⚠️ GLFW — СТРОГО ВТОРЫМ
#include <GLFW/glfw3.h>

#include "mine/core/Application.hpp"
#include "mine/graphics/Renderer.hpp"
#include "mine/graphics/Texture.hpp"
#include "mine/math/Vector2.hpp"
#include "mine/scene/TileMap.hpp"
#include "mine/utils/TextureManager.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace mine {

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

// ─────────────────────── ctor / dtor ───────────────────────

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

  m_assetsDir = expandPath("~/Projects/XENOCIDE/assets");

  // Player texture
  m_playerTexture =
      m_texManager.load(m_assetsDir + "/textures/player.png");

  // Load tile maps
  loadMaps();
}

Application::~Application() {
  delete m_renderer;
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

// ─────────────────────── map loading ───────────────────────

void Application::loadMaps() {
  std::string mapDir = m_assetsDir + "/maps/";

  m_floors.resize(3);
  const char *files[] = {"floor0.map", "floor1.map", "floor2.map"};

  for (int i = 0; i < 3; ++i) {
    if (!m_floors[i].load(mapDir + files[i], m_texManager, m_assetsDir)) {
      std::cerr << "⚠️  Failed to load " << files[i] << "\n";
    }
  }

  // Spawn on ground floor (floor index 1)
  mCurrentFloor = 1;
  if (mCurrentFloor < static_cast<int>(m_floors.size())) {
    m_playerPos = m_floors[mCurrentFloor].findSpawn();
  } else {
    m_playerPos = {1000.0f, 1000.0f};
  }
  m_cameraPos = m_playerPos;
}

// ─────────────────────── collision ─────────────────────────

bool Application::checkWallCollision(const Vector2 &newPos, float radius) {
  if (mCurrentFloor < 0 || mCurrentFloor >= static_cast<int>(m_floors.size()))
    return false;
  return m_floors[mCurrentFloor].checkCollision(newPos, radius);
}

// ─────────────────────── input ─────────────────────────────

void Application::processInput(float deltaTime) {
  constexpr float speed = 300.0f;
  Vector2 desiredPos = m_playerPos;

  if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    m_running = false;

  // Camera shake
  if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    if (!m_hasShaken) {
      shakeCamera(10.0f, 0.3f);
      m_hasShaken = true;
    }
  } else {
    m_hasShaken = false;
  }

  // WASD movement
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

  // Per-axis collision (slide along walls)
  if (!checkWallCollision({desiredPos.x, m_playerPos.y}, 30.0f))
    m_playerPos.x = desiredPos.x;
  if (!checkWallCollision({m_playerPos.x, desiredPos.y}, 30.0f))
    m_playerPos.y = desiredPos.y;

  // Clamp to world bounds
  if (mCurrentFloor >= 0 && mCurrentFloor < static_cast<int>(m_floors.size())) {
    float ww = m_floors[mCurrentFloor].getWorldWidth();
    float wh = m_floors[mCurrentFloor].getWorldHeight();
    m_playerPos.x = std::clamp(m_playerPos.x, 32.0f, ww - 32.0f);
    m_playerPos.y = std::clamp(m_playerPos.y, 32.0f, wh - 32.0f);
  }

  // ── Mouse aim → rotation (FIXED: was + PI/2, now - PI/2) ──
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
  constexpr float PI = 3.14159265358979323846f;
  m_playerRotation = angle - (PI * 0.5f);

  // ── Elevator interaction (E tiles) ──
  mNearElevator = false;
  if (mCurrentFloor >= 0 && mCurrentFloor < static_cast<int>(m_floors.size())) {
    auto elevPositions = m_floors[mCurrentFloor].findTiles('E');
    for (const auto &ep : elevPositions) {
      float dx = m_playerPos.x - ep.x;
      float dy = m_playerPos.y - ep.y;
      if (dx * dx + dy * dy < 120.0f * 120.0f) {
        mNearElevator = true;
        break;
      }
    }
  }

  if (!m_editorMode && mNearElevator && !mElevatorCooldown) {
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
      std::cout << "🛗 Floor " << (mCurrentFloor + 1) << "\n";
    }
  }
  if (glfwGetKey(m_window, GLFW_KEY_1) == GLFW_RELEASE &&
      glfwGetKey(m_window, GLFW_KEY_2) == GLFW_RELEASE &&
      glfwGetKey(m_window, GLFW_KEY_3) == GLFW_RELEASE)
    mElevatorCooldown = false;

  // ── Stair interaction (S tiles) ──
  bool nearStair = false;
  if (mCurrentFloor >= 0 && mCurrentFloor < static_cast<int>(m_floors.size())) {
    auto stairPositions = m_floors[mCurrentFloor].findTiles('S');
    for (const auto &sp : stairPositions) {
      float dx = m_playerPos.x - sp.x;
      float dy = m_playerPos.y - sp.y;
      if (dx * dx + dy * dy < 120.0f * 120.0f) {
        nearStair = true;
        break;
      }
    }
  }

  if (nearStair) {
    if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS && !mElevatorCooldown) {
      if (mCurrentFloor < 2) {
        mCurrentFloor++;
        mElevatorCooldown = true;
        std::cout << "🪜 UP → floor " << (mCurrentFloor + 1) << "\n";
      }
    }
    if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS && !mElevatorCooldown) {
      if (mCurrentFloor > 0) {
        mCurrentFloor--;
        mElevatorCooldown = true;
        std::cout << "🪜 DOWN → floor " << (mCurrentFloor + 1) << "\n";
      }
    }
    if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_RELEASE &&
        glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_RELEASE)
      mElevatorCooldown = false;
  }

  // ── Editor toggle (F1) ──
  if (glfwGetKey(m_window, GLFW_KEY_F1) == GLFW_PRESS) {
    if (!m_f1Pressed) {
      m_editorMode = !m_editorMode;
      m_f1Pressed = true;
      std::cout << (m_editorMode ? "📝 Editor ON  (LMB=place RMB=erase []=cycle F5=save)\n"
                                 : "📝 Editor OFF\n");
    }
  } else {
    m_f1Pressed = false;
  }

  // ── Editor input ──
  if (m_editorMode && mCurrentFloor >= 0 &&
      mCurrentFloor < static_cast<int>(m_floors.size())) {

    auto &map = m_floors[mCurrentFloor];
    const auto &defs = map.getTileDefs();
    int numDefs = static_cast<int>(defs.size());

    // Bracket keys cycle tile selection
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
      if (!m_bracketPressed) {
        m_editorTileIndex = (m_editorTileIndex + 1) % numDefs;
        if (m_editorTileIndex == 0)
          m_editorTileIndex = 1; // skip empty
        m_bracketPressed = true;
        std::cout << "🎨 Tile: '" << defs[m_editorTileIndex].symbol << "'\n";
      }
    } else if (glfwGetKey(m_window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
      if (!m_bracketPressed) {
        m_editorTileIndex--;
        if (m_editorTileIndex <= 0)
          m_editorTileIndex = numDefs - 1;
        m_bracketPressed = true;
        std::cout << "🎨 Tile: '" << defs[m_editorTileIndex].symbol << "'\n";
      }
    } else {
      m_bracketPressed = false;
    }

    // Clamp tile index
    if (m_editorTileIndex >= numDefs)
      m_editorTileIndex = 1;

    // Mouse world position
    float worldMouseX =
        m_cameraPos.x + (static_cast<float>(mouseX) - mWindowWidth / 2.0f);
    float worldMouseY =
        m_cameraPos.y + (mWindowHeight / 2.0f - static_cast<float>(mouseY));
    auto [gx, gy] = map.worldToGrid({worldMouseX, worldMouseY});

    // LMB → place tile
    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      if (gx >= 0 && gx < map.getWidth() && gy >= 0 && gy < map.getHeight())
        map.setTile(gx, gy, defs[m_editorTileIndex].symbol);
    }
    // RMB → erase (set to floor '.')
    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
      if (gx >= 0 && gx < map.getWidth() && gy >= 0 && gy < map.getHeight())
        map.setTile(gx, gy, '.');
    }

    // F5 → save current floor
    if (glfwGetKey(m_window, GLFW_KEY_F5) == GLFW_PRESS) {
      if (!m_f5Pressed) {
        std::string mapDir = m_assetsDir + "/maps/";
        const char *files[] = {"floor0.map", "floor1.map", "floor2.map"};
        map.save(mapDir + files[mCurrentFloor]);
        m_f5Pressed = true;
      }
    } else {
      m_f5Pressed = false;
    }
  }
}

// ─────────────────────── update ────────────────────────────

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

// ─────────────────────── editor overlay ────────────────────

void Application::renderEditorOverlay() {
  if (!m_editorMode)
    return;
  if (mCurrentFloor < 0 || mCurrentFloor >= static_cast<int>(m_floors.size()))
    return;

  const auto &map = m_floors[mCurrentFloor];
  float ts = map.getTileSize();
  float worldW = map.getWorldWidth();
  float worldH = map.getWorldHeight();

  // Grid lines (only visible area)
  float lineW = 2.0f;
  float viewL = m_cameraPos.x - mWindowWidth / 2.0f;
  float viewR = m_cameraPos.x + mWindowWidth / 2.0f;
  float viewB = m_cameraPos.y - mWindowHeight / 2.0f;
  float viewT = m_cameraPos.y + mWindowHeight / 2.0f;

  int colStart = std::max(0, static_cast<int>(viewL / ts));
  int colEnd = std::min(map.getWidth(), static_cast<int>(viewR / ts) + 1);
  for (int c = colStart; c <= colEnd; ++c) {
    float x = c * ts;
    m_renderer->drawQuad({x, worldH / 2.0f}, {lineW, worldH}, 0.35f, 0.35f,
                         0.45f, 0.0f);
  }

  int rowStart = std::max(0, static_cast<int>((worldH - viewT) / ts));
  int rowEnd =
      std::min(map.getHeight(), static_cast<int>((worldH - viewB) / ts) + 1);
  for (int r = rowStart; r <= rowEnd; ++r) {
    float y = worldH - r * ts;
    m_renderer->drawQuad({worldW / 2.0f, y}, {worldW, lineW}, 0.35f, 0.35f,
                         0.45f, 0.0f);
  }

  // Cursor highlight (yellow outline)
  double mx, my;
  glfwGetCursorPos(m_window, &mx, &my);
  float wmx = m_cameraPos.x + (static_cast<float>(mx) - mWindowWidth / 2.0f);
  float wmy = m_cameraPos.y + (mWindowHeight / 2.0f - static_cast<float>(my));
  auto [gx, gy] = map.worldToGrid({wmx, wmy});

  if (gx >= 0 && gx < map.getWidth() && gy >= 0 && gy < map.getHeight()) {
    Vector2 tp = map.gridToWorld(gx, gy);
    float half = ts / 2.0f;
    float bw = 3.0f;
    // top
    m_renderer->drawQuad({tp.x, tp.y + half - bw / 2.0f}, {ts, bw}, 1.0f,
                         1.0f, 0.0f, 0.0f);
    // bottom
    m_renderer->drawQuad({tp.x, tp.y - half + bw / 2.0f}, {ts, bw}, 1.0f,
                         1.0f, 0.0f, 0.0f);
    // left
    m_renderer->drawQuad({tp.x - half + bw / 2.0f, tp.y}, {bw, ts}, 1.0f,
                         1.0f, 0.0f, 0.0f);
    // right
    m_renderer->drawQuad({tp.x + half - bw / 2.0f, tp.y}, {bw, ts}, 1.0f,
                         1.0f, 0.0f, 0.0f);
  }

  // ── Palette HUD (screen space) ──
  m_renderer->beginScreenSpace();

  const auto &defs = map.getTileDefs();
  float palX = 20.0f;
  float palY = 20.0f;
  float boxSize = 32.0f;
  float gap = 6.0f;

  // Background bar
  float barW = defs.size() * (boxSize + gap) + gap;
  m_renderer->drawQuad({palX + barW / 2.0f - gap / 2.0f, palY + boxSize / 2.0f},
                       {barW, boxSize + 12.0f}, 0.08f, 0.08f, 0.12f, 0.0f);

  for (int i = 0; i < static_cast<int>(defs.size()); ++i) {
    float bx = palX + i * (boxSize + gap) + boxSize / 2.0f;
    float by = palY + boxSize / 2.0f;

    // Selection highlight
    if (i == m_editorTileIndex) {
      m_renderer->drawQuad({bx, by}, {boxSize + 6.0f, boxSize + 6.0f}, 1.0f,
                           1.0f, 0.0f, 0.0f);
    }

    // Tile color swatch
    if (defs[i].symbol == ' ') {
      m_renderer->drawQuad({bx, by}, {boxSize, boxSize}, 0.15f, 0.15f, 0.2f,
                           0.0f);
    } else {
      m_renderer->drawQuad({bx, by}, {boxSize, boxSize}, defs[i].r, defs[i].g,
                           defs[i].b, 0.0f);
    }
  }

  // "EDITOR" indicator in top-right
  m_renderer->drawQuad(
      {static_cast<float>(mWindowWidth) - 60.0f,
       static_cast<float>(mWindowHeight) - 20.0f},
      {100, 28}, 0.9f, 0.2f, 0.2f, 0.0f);

  // Restore camera projection
  Vector2 shake{0.0f, 0.0f};
  m_renderer->setCameraPosition(m_cameraPos, shake);
}

// ─────────────────────── render ────────────────────────────

void Application::render() {
  Vector2 shakeOffset{0.0f, 0.0f};
  if (m_cameraShakeIntensity > 0.0f) {
    shakeOffset.x = m_randomDist(m_randomEngine) * m_cameraShakeIntensity;
    shakeOffset.y = m_randomDist(m_randomEngine) * m_cameraShakeIntensity;
  }

  m_renderer->beginFrame();
  m_renderer->setCameraPosition(m_cameraPos, shakeOffset);

  // Draw current floor tile map
  if (mCurrentFloor >= 0 && mCurrentFloor < static_cast<int>(m_floors.size())) {
    m_floors[mCurrentFloor].render(*m_renderer);

    // Highlight elevator tiles (E)
    auto elevPositions = m_floors[mCurrentFloor].findTiles('E');
    for (const auto &ep : elevPositions) {
      float dx = m_playerPos.x - ep.x;
      float dy = m_playerPos.y - ep.y;
      bool nearby = (dx * dx + dy * dy < 150.0f * 150.0f);
      m_renderer->drawQuad(ep, {80, 80}, nearby ? 0.9f : 0.5f,
                           nearby ? 0.3f : 0.3f, nearby ? 0.4f : 0.7f, 0.0f);
    }

    // Highlight stair tiles (S) — draw 3 step lines
    auto stairPositions = m_floors[mCurrentFloor].findTiles('S');
    for (const auto &sp : stairPositions) {
      float dx = m_playerPos.x - sp.x;
      float dy = m_playerPos.y - sp.y;
      bool nearby = (dx * dx + dy * dy < 150.0f * 150.0f);
      for (int i = 0; i < 3; ++i) {
        m_renderer->drawQuad({sp.x, sp.y - 20.0f + i * 15.0f}, {100, 8},
                             nearby ? 0.8f : 0.6f, nearby ? 0.6f : 0.4f,
                             nearby ? 0.2f : 0.3f, 0.0f);
      }
    }
  }

  // Player
  if (m_playerTexture) {
    m_renderer->drawQuad(m_playerPos, {64, 64}, m_playerTexture.get(),
                         m_playerRotation);
  } else {
    m_renderer->drawQuad(m_playerPos, {64, 64}, 0.9f, 0.2f, 0.8f,
                         m_playerRotation);
  }

  // Editor overlay (grid + palette)
  renderEditorOverlay();

  // ── UI: elevator panel (when nearby) ──
  if (mNearElevator) {
    // Compute world position that maps to player's screen position
    float uiWX = m_playerPos.x;
    float uiWY = m_playerPos.y + 120.0f;

    m_renderer->drawQuad({uiWX, uiWY}, {200, 120}, 0.1f, 0.1f, 0.15f, 0.0f);
    m_renderer->drawQuad({uiWX, uiWY}, {196, 116}, 0.2f, 0.2f, 0.25f, 0.0f);

    for (int i = 0; i < 3; ++i) {
      float btnY = uiWY - 30.0f + i * 35.0f;
      bool selected = (mSelectedFloor == i);
      m_renderer->drawQuad({uiWX, btnY}, {150, 25}, selected ? 0.9f : 0.3f,
                           selected ? 0.3f : 0.3f, selected ? 0.4f : 0.3f,
                           0.0f);
      m_renderer->drawQuad({uiWX - 50.0f, btnY}, {30, 15}, 1.0f, 1.0f, 1.0f,
                           0.0f);
    }
    m_renderer->drawQuad({uiWX, uiWY + 50.0f}, {180, 15}, 0.7f, 0.7f, 0.7f,
                         0.0f);
  }

  // ── UI: floor indicator (screen space) ──
  {
    m_renderer->beginScreenSpace();

    float uiX = 50.0f;
    float uiY = static_cast<float>(mWindowHeight) - 50.0f;
    m_renderer->drawQuad({uiX, uiY}, {220, 40}, 0.05f, 0.05f, 0.1f, 0.0f);

    const float floorColors[3][3] = {
        {0.15f, 0.10f, 0.25f}, {0.20f, 0.25f, 0.15f}, {0.25f, 0.15f, 0.15f}};

    for (int i = 0; i < 3; ++i) {
      float labelX = uiX + i * 70.0f + 20.0f;
      if (i == mCurrentFloor) {
        m_renderer->drawQuad({labelX, uiY}, {50, 30},
                             floorColors[i][0] * 1.8f,
                             floorColors[i][1] * 1.8f,
                             floorColors[i][2] * 1.8f, 0.0f);
      }
      m_renderer->drawQuad({labelX - 15.0f, uiY}, {30, 20}, 1.0f, 1.0f, 1.0f,
                           0.0f);
    }

    // Restore camera
    m_renderer->setCameraPosition(m_cameraPos, shakeOffset);
  }

  m_renderer->endFrame();
}

// ─────────────────────── run ───────────────────────────────

void Application::run() {
  std::cout << "\n🎮 XENOCIDE running!\n";
  std::cout << "   WASD/Arrows — move | Mouse — aim | SPACE — shake\n";
  std::cout << "   1/2/3 — elevator floors | E/Q — stairs up/down\n";
  std::cout << "   F1 — toggle map editor | ESC — exit\n";
  std::cout << "   [Editor] LMB — place | RMB — erase | [] — cycle tiles | "
               "F5 — save\n\n";
  std::cout << "   To add textures: put images in assets/textures/\n";
  std::cout << "   then reference them in .map files:\n";
  std::cout << "     tile T 1 0.5 0.5 0.5 mytexture.png\n\n";

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
