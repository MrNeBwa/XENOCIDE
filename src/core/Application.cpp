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
#include "miniaudio.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace mine {

static constexpr float PI = 3.14159265358979323846f;

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
  m_playerTexture = m_texManager.load(m_assetsDir + "/textures/player.png");

  // Load tile maps
  loadMaps();

  // Spawn aliens on all floors
  spawnAliens();

  // Init audio and play music
  initAudio();
}

Application::~Application() {
  shutdownAudio();
  delete m_renderer;
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

// ─────────────────────── audio ─────────────────────────────

void Application::initAudio() {
  m_audioEngine = new ma_engine();
  ma_result result = ma_engine_init(nullptr, m_audioEngine);
  if (result != MA_SUCCESS) {
    std::cerr << "⚠️  Audio engine init failed (code " << result << ")\n";
    delete m_audioEngine;
    m_audioEngine = nullptr;
    return;
  }

  // Create a looping sound instead of fire-and-forget
  std::string musicPath = m_assetsDir + "/music/a.mp3";
  m_musicSound = new ma_sound();
  result = ma_sound_init_from_file(m_audioEngine, musicPath.c_str(),
                                   MA_SOUND_FLAG_STREAM, nullptr, nullptr,
                                   m_musicSound);
  if (result == MA_SUCCESS) {
    ma_sound_set_looping(m_musicSound, MA_TRUE);
    ma_sound_set_volume(m_musicSound, 0.5f);
    ma_sound_start(m_musicSound);
    m_musicPlaying = true;
    std::cout << "🎵 Background music playing (looped)\n";
  } else {
    std::cerr << "⚠️  Could not load " << musicPath << " (code " << result << ")\n";
    std::cerr << "   Place your MP3 file at assets/music/a.mp3\n";
    delete m_musicSound;
    m_musicSound = nullptr;
  }
}

void Application::shutdownAudio() {
  if (m_musicSound) {
    ma_sound_uninit(m_musicSound);
    delete m_musicSound;
    m_musicSound = nullptr;
  }
  if (m_audioEngine) {
    ma_engine_uninit(m_audioEngine);
    delete m_audioEngine;
    m_audioEngine = nullptr;
  }
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

  // Find rabbit position on floor 2 (3rd floor) — look for 'B' tile
  if (m_floors.size() > 2) {
    auto rabbitTiles = m_floors[2].findTiles('B');
    if (!rabbitTiles.empty()) {
      m_rabbitPos = rabbitTiles[0];
    } else {
      // Fallback: place rabbit in center of floor 2
      m_rabbitPos = {m_floors[2].getWorldWidth() / 2.0f,
                     m_floors[2].getWorldHeight() / 2.0f};
    }
  }

  // Find exit position on floor 0 (1st floor) — look for 'X' tile
  if (!m_floors.empty()) {
    auto exitTiles = m_floors[0].findTiles('X');
    if (!exitTiles.empty()) {
      m_exitPos = exitTiles[0];
    } else {
      m_exitPos = {150.0f, 150.0f};
    }
  }
}

// ─────────────────────── alien spawning ────────────────────

void Application::spawnAliens() {
  m_aliens.clear();

  // Spawn aliens at 'A' tiles on each floor
  for (int f = 0; f < static_cast<int>(m_floors.size()); ++f) {
    auto spawnPositions = m_floors[f].findTiles('A');
    for (const auto &sp : spawnPositions) {
      Alien alien;
      alien.pos = sp;
      alien.floor = f;
      alien.patrolTarget = sp;
      alien.idleTimer = m_randomDist(m_randomEngine) * 2.0f + 1.0f;
      alien.speed = 70.0f + m_randomDist(m_randomEngine) * 30.0f;
      alien.sightRange = 350.0f + m_randomDist(m_randomEngine) * 100.0f;
      alien.animTimer = m_randomDist(m_randomEngine) * 6.28f;
      alien.hp = 35.0f + std::abs(m_randomDist(m_randomEngine)) * 25.0f;
      alien.maxHp = alien.hp;
      m_aliens.push_back(alien);
    }
  }

  std::cout << "👾 Spawned " << m_aliens.size() << " aliens across all floors\n";
}

// ─────────────────────── collision helpers ─────────────────

bool Application::checkWallCollision(const Vector2 &newPos, float radius) {
  if (mCurrentFloor < 0 || mCurrentFloor >= static_cast<int>(m_floors.size()))
    return false;
  return m_floors[mCurrentFloor].checkCollision(newPos, radius);
}

bool Application::checkWallCollisionOnFloor(int floor, const Vector2 &pos,
                                            float radius) {
  if (floor < 0 || floor >= static_cast<int>(m_floors.size()))
    return false;
  return m_floors[floor].checkCollision(pos, radius);
}

// ─────────────────────── line of sight ─────────────────────

bool Application::hasLineOfSight(int floor, const Vector2 &from,
                                 const Vector2 &to) {
  if (floor < 0 || floor >= static_cast<int>(m_floors.size()))
    return false;

  Vector2 dir = to - from;
  float dist = dir.length();
  if (dist < 1.0f)
    return true;

  Vector2 step = dir.normalized() * 20.0f;
  int steps = static_cast<int>(dist / 20.0f);
  Vector2 current = from;

  for (int i = 0; i < steps; ++i) {
    current += step;
    if (m_floors[floor].checkCollision(current, 5.0f))
      return false;
  }
  return true;
}

// ─────────────────────── player attack ─────────────────────

void Application::playerAttack() {
  // Find closest alive alien on current floor in front of player
  float bestDist = m_playerAttackRange;
  Alien *target = nullptr;

  float facingAngle = m_playerRotation + PI * 0.5f;
  Vector2 facingDir{std::cos(facingAngle), std::sin(facingAngle)};

  for (auto &alien : m_aliens) {
    if (!alien.alive || alien.floor != mCurrentFloor)
      continue;
    if (alien.state == Alien::State::Dying)
      continue;

    float dist = Vector2::distance(m_playerPos, alien.pos);
    if (dist > m_playerAttackRange)
      continue;

    // Check facing direction (120-degree cone in front)
    Vector2 toAlien = (alien.pos - m_playerPos).normalized();
    float dot = Vector2::dot(facingDir, toAlien);
    if (dot < 0.3f) // ~70 degree half-cone
      continue;

    if (dist < bestDist) {
      bestDist = dist;
      target = &alien;
    }
  }

  if (target) {
    // Start lunge toward alien
    m_isLunging = true;
    m_lungeTimer = 0.0f;
    m_lungeStart = m_playerPos;
    m_lungeTarget = target->pos;
    m_playerAttackCooldown = 0.4f;

    // Deal damage
    target->hp -= m_playerAttackDamage;
    shakeCamera(6.0f, 0.15f);

    // Blood on hit
    spawnBlood(target->pos, 12);

    if (target->hp <= 0.0f) {
      // Kill the alien — start death animation
      target->state = Alien::State::Dying;
      target->deathTimer = 0.0f;
      target->squash = 1.0f;
      m_killCount++;

      // Big blood splatter on death!
      spawnBlood(target->pos, 40);
      shakeCamera(12.0f, 0.25f);
      std::cout << "💀 Alien killed! (" << m_killCount << " total)\n";
    } else {
      // Hurt feedback
      spawnParticles(target->pos, 5, 0.8f, 0.1f, 0.1f);
    }
  } else {
    // Swing and miss — still do a small camera effect
    shakeCamera(3.0f, 0.1f);
    m_playerAttackCooldown = 0.3f;
  }
}

void Application::spawnBlood(const Vector2 &pos, int count) {
  // Blood drops — big, red, with gravity
  spawnParticlesEx(pos, count,
                   0.7f, 0.0f, 0.0f,         // dark red
                   4.0f, 14.0f,               // size range
                   60.0f, 200.0f,             // speed range
                   0.5f, 1.5f,                // life range
                   150.0f);                   // gravity

  // Smaller bright red specks
  spawnParticlesEx(pos, count / 2,
                   1.0f, 0.15f, 0.1f,         // bright red
                   2.0f, 6.0f,                // size
                   100.0f, 300.0f,            // speed
                   0.3f, 0.8f,                // life
                   200.0f);                   // gravity
}

// ─────────────────────── input ─────────────────────────────

void Application::processInput(float deltaTime) {
  if (m_gameState != GameState::Playing) {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      m_running = false;
    if (glfwGetKey(m_window, GLFW_KEY_R) == GLFW_PRESS) {
      m_gameState = GameState::Playing;
      m_playerHP = m_playerMaxHP;
      m_playerStamina = m_playerMaxStamina;
      m_hasRabbit = false;
      m_rabbitPickedUp = false;
      m_gameTime = 0.0f;
      m_winTimer = 0.0f;
      m_damageFlashTimer = 0.0f;
      m_killCount = 0;
      m_playerAttackCooldown = 0.0f;
      m_isLunging = false;
      m_lungeHeight = 0.0f;
      m_particles.clear();
      m_hints.clear();
      m_shownStartHint = false;
      m_shownRabbitHint = false;
      m_shownDoorHint = false;
      m_shownAttackHint = false;
      m_lastHintFloor = -1;
      m_hintCooldown = 0.0f;
      loadMaps();
      spawnAliens();
      // Restart music
      if (m_musicSound) {
        ma_sound_seek_to_pcm_frame(m_musicSound, 0);
        ma_sound_start(m_musicSound);
      }
    }
    return;
  }

  // ── Sprint detection ──
  bool shiftHeld = glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                   glfwGetKey(m_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
  bool wantsSprint = shiftHeld && m_playerStamina > 0.0f;

  bool isMoving = false;
  float baseSpeed = 300.0f;
  float sprintMultiplier = 1.8f;
  float speed = wantsSprint ? baseSpeed * sprintMultiplier : baseSpeed;

  Vector2 desiredPos = m_playerPos;

  if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    m_running = false;

  // ── Player attack on Space ──
  if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    if (!m_spacePressed && m_playerAttackCooldown <= 0.0f && !m_isLunging) {
      m_spacePressed = true;
      playerAttack();
    }
  } else {
    m_spacePressed = false;
  }
  m_playerAttackCooldown -= deltaTime;

  // ── Lunge animation update ──
  if (m_isLunging) {
    m_lungeTimer += deltaTime;
    float t = m_lungeTimer / m_lungeDuration;
    if (t >= 1.0f) {
      m_isLunging = false;
      m_lungeHeight = 0.0f;
      m_lungeTimer = 0.0f;
    } else {
      // Parabolic jump arc
      m_lungeHeight = std::sin(t * PI) * 40.0f;
      // Interpolate position toward target
      float lerpT = std::min(t * 2.0f, 1.0f);
      Vector2 lungePos = m_lungeStart + (m_lungeTarget - m_lungeStart) * lerpT;
      if (!checkWallCollision(lungePos, 30.0f)) {
        m_playerPos = lungePos;
      }
    }
  }

  // WASD movement
  Vector2 moveDir{0.0f, 0.0f};
  if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
    moveDir.x -= 1.0f;
  if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
    moveDir.x += 1.0f;
  if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
    moveDir.y += 1.0f;
  if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
    moveDir.y -= 1.0f;

  // Normalize diagonal movement
  if (moveDir.lengthSq() > 0.01f) {
    moveDir = moveDir.normalized();
    isMoving = true;
  }

  desiredPos.x += moveDir.x * speed * deltaTime;
  desiredPos.y += moveDir.y * speed * deltaTime;

  // Per-axis collision (slide along walls)
  if (!checkWallCollision({desiredPos.x, m_playerPos.y}, 30.0f))
    m_playerPos.x = desiredPos.x;
  if (!checkWallCollision({m_playerPos.x, desiredPos.y}, 30.0f))
    m_playerPos.y = desiredPos.y;

  // Clamp to world bounds
  if (mCurrentFloor >= 0 &&
      mCurrentFloor < static_cast<int>(m_floors.size())) {
    float ww = m_floors[mCurrentFloor].getWorldWidth();
    float wh = m_floors[mCurrentFloor].getWorldHeight();
    m_playerPos.x = std::clamp(m_playerPos.x, 32.0f, ww - 32.0f);
    m_playerPos.y = std::clamp(m_playerPos.y, 32.0f, wh - 32.0f);
  }

  // ── Sprint stamina management ──
  m_isSprinting = wantsSprint && isMoving;
  if (m_isSprinting) {
    m_playerStamina -= m_staminaDrainRate * deltaTime;
    m_playerStamina = std::max(0.0f, m_playerStamina);
    m_staminaRegenTimer = m_staminaRegenDelay;
  } else {
    m_staminaRegenTimer -= deltaTime;
    if (m_staminaRegenTimer <= 0.0f) {
      m_playerStamina += m_staminaRegenRate * deltaTime;
      m_playerStamina = std::min(m_playerStamina, m_playerMaxStamina);
    }
  }

  // ── Footstep particles while moving ──
  if (isMoving) {
    m_footstepTimer += deltaTime;
    float interval = m_isSprinting ? 0.08f : 0.15f;
    if (m_footstepTimer >= interval) {
      m_footstepTimer = 0.0f;
      m_stepCount++;
      spawnParticles(m_playerPos, 1, 0.4f, 0.35f, 0.3f);
    }
  }

  // ── Mouse aim → rotation (Wayland-safe: normalized coordinates) ──
  {
    int winWidth, winHeight;
    glfwGetWindowSize(m_window, &winWidth, &winHeight);
    double mouseX, mouseY;
    glfwGetCursorPos(m_window, &mouseX, &mouseY);

    // Normalize cursor to [0,1] then map to projection space
    // This works correctly on both X11 and Wayland/HiDPI
    float normX = (winWidth > 0) ? static_cast<float>(mouseX) / static_cast<float>(winWidth) : 0.5f;
    float normY = (winHeight > 0) ? static_cast<float>(mouseY) / static_cast<float>(winHeight) : 0.5f;

    float screenMouseX = normX * static_cast<float>(mWindowWidth);
    float screenMouseY = (1.0f - normY) * static_cast<float>(mWindowHeight);  // flip Y

    float screenPlayerX = (m_playerPos.x - m_cameraPos.x) + (mWindowWidth * 0.5f);
    float screenPlayerY = (m_playerPos.y - m_cameraPos.y) + (mWindowHeight * 0.5f);

    float dx = screenMouseX - screenPlayerX;
    float dy = screenMouseY - screenPlayerY;
    if (dx * dx + dy * dy > 1.0f) {
      m_playerRotation = std::atan2(dy, dx) - (PI * 0.5f);
    }
  }

  // ── Rabbit pickup (floor 2, tile 'B') ──
  if (!m_hasRabbit && mCurrentFloor == m_rabbitFloor) {
    float rabbitDist = Vector2::distanceSq(m_playerPos, m_rabbitPos);
    if (rabbitDist < 80.0f * 80.0f) {
      if (glfwGetKey(m_window, GLFW_KEY_F) == GLFW_PRESS) {
        m_hasRabbit = true;
        m_rabbitPickedUp = true;
        shakeCamera(5.0f, 0.2f);
        spawnParticles(m_rabbitPos, 20, 1.0f, 1.0f, 1.0f);
        std::cout << "🐇 Rabbit rescued! Now escape through the DOOR on floor 1!\n";
        std::cout << "   ⚠️  The aliens don't look happy about this...\n";
        showHint(4.0f, 1.0f, 0.4f, 0.2f, 1);

        // Aliens become more aggressive
        for (auto &alien : m_aliens) {
          alien.sightRange *= 1.8f;
          alien.speed *= 1.4f;
          alien.damage *= 1.5f;
          alien.sightAngle = PI; // near 180 degrees
        }
      }
    }
  }

  // ── Exit check (floor 0, tile 'X') ──
  if (m_hasRabbit && mCurrentFloor == m_exitFloor) {
    float exitDist = Vector2::distanceSq(m_playerPos, m_exitPos);
    if (exitDist < 100.0f * 100.0f) {
      m_gameState = GameState::Won;
      std::cout << "\n🎉 YOU WIN! Rabbit saved in " << m_gameTime << "s!\n";
      std::cout << "   Aliens stomped: " << m_killCount << "\n";
      if (m_killCount == 0)
        std::cout << "   🕊️  Pacifist run! Not a single alien harmed!\n";
      else if (m_killCount >= 10)
        std::cout << "   🔥 ALIEN SLAYER! They never stood a chance!\n";
      std::cout << "   Press R to play again!\n\n";
      spawnParticles(m_playerPos, 50, 0.2f, 1.0f, 0.3f);
    }
  }

  // ── Elevator interaction (E tiles) ──
  mNearElevator = false;
  if (mCurrentFloor >= 0 &&
      mCurrentFloor < static_cast<int>(m_floors.size())) {
    auto elevPositions = m_floors[mCurrentFloor].findTiles('E');
    for (const auto &ep : elevPositions) {
      if (Vector2::distanceSq(m_playerPos, ep) < 120.0f * 120.0f) {
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
  if (mCurrentFloor >= 0 &&
      mCurrentFloor < static_cast<int>(m_floors.size())) {
    auto stairPositions = m_floors[mCurrentFloor].findTiles('S');
    for (const auto &sp : stairPositions) {
      if (Vector2::distanceSq(m_playerPos, sp) < 120.0f * 120.0f) {
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
      std::cout << (m_editorMode
                        ? "📝 Editor ON  (LMB=place RMB=erase []=cycle "
                          "F5=save)\n"
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

    if (glfwGetKey(m_window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
      if (!m_bracketPressed) {
        m_editorTileIndex = (m_editorTileIndex + 1) % numDefs;
        if (m_editorTileIndex == 0)
          m_editorTileIndex = 1;
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

    if (m_editorTileIndex >= numDefs)
      m_editorTileIndex = 1;

    double edMouseX, edMouseY;
    glfwGetCursorPos(m_window, &edMouseX, &edMouseY);
    float worldMouseX =
        m_cameraPos.x + (static_cast<float>(edMouseX) - mWindowWidth / 2.0f);
    float worldMouseY =
        m_cameraPos.y + (mWindowHeight / 2.0f - static_cast<float>(edMouseY));
    auto [gx, gy] = map.worldToGrid({worldMouseX, worldMouseY});

    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      if (gx >= 0 && gx < map.getWidth() && gy >= 0 && gy < map.getHeight())
        map.setTile(gx, gy, defs[m_editorTileIndex].symbol);
    }
    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
      if (gx >= 0 && gx < map.getWidth() && gy >= 0 && gy < map.getHeight())
        map.setTile(gx, gy, '.');
    }

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

// ─────────────────────── alien AI ──────────────────────────

void Application::updateAliens(float deltaTime) {
  for (auto &alien : m_aliens) {
    if (!alien.alive)
      continue;

    // Handle dying animation
    if (alien.state == Alien::State::Dying) {
      alien.deathTimer += deltaTime;
      alien.squash = 1.0f - (alien.deathTimer / alien.deathDuration);
      alien.squash = std::max(0.0f, alien.squash);

      // Drip blood during death
      if (static_cast<int>(alien.deathTimer * 20.0f) % 3 == 0) {
        spawnBlood(alien.pos, 2);
      }

      if (alien.deathTimer >= alien.deathDuration) {
        alien.alive = false;
      }
      continue;
    }

    if (alien.floor != mCurrentFloor) {
      // Off-screen aliens still idle-tick
      alien.idleTimer -= deltaTime;
      alien.animTimer += deltaTime;
      continue;
    }

    alien.animTimer += deltaTime;
    alien.attackCooldown -= deltaTime;

    float distToPlayer = Vector2::distance(alien.pos, m_playerPos);
    Vector2 toPlayer = m_playerPos - alien.pos;

    // Check if alien can see the player
    bool canSeePlayer = false;
    if (distToPlayer < alien.sightRange) {
      // Field-of-view check
      float alienFacing = alien.rotation + PI * 0.5f;
      Vector2 facingDir{std::cos(alienFacing), std::sin(alienFacing)};
      Vector2 toPlayerNorm = toPlayer.normalized();
      float dot = Vector2::dot(facingDir, toPlayerNorm);
      if (dot > std::cos(alien.sightAngle)) {
        canSeePlayer =
            hasLineOfSight(alien.floor, alien.pos, m_playerPos);
      }
    }

    // If rabbit is picked up, aliens are more aggressive
    if (m_rabbitPickedUp && distToPlayer < alien.sightRange) {
      canSeePlayer =
          hasLineOfSight(alien.floor, alien.pos, m_playerPos);
    }

    switch (alien.state) {
    case Alien::State::Idle: {
      alien.idleTimer -= deltaTime;
      if (alien.idleTimer <= 0.0f) {
        alien.state = Alien::State::Patrol;
        float randAngle = m_randomDist(m_randomEngine) * PI;
        float randDist =
            100.0f + std::abs(m_randomDist(m_randomEngine)) * 200.0f;
        alien.patrolTarget =
            alien.pos +
            Vector2{std::cos(randAngle) * randDist,
                    std::sin(randAngle) * randDist};
      }
      if (canSeePlayer) {
        alien.state = Alien::State::Chase;
      }
      break;
    }

    case Alien::State::Patrol: {
      Vector2 toTarget = alien.patrolTarget - alien.pos;
      float distTarget = toTarget.length();
      if (distTarget < 20.0f) {
        alien.state = Alien::State::Idle;
        alien.idleTimer =
            1.0f + std::abs(m_randomDist(m_randomEngine)) * 3.0f;
      } else {
        Vector2 md = toTarget.normalized();
        Vector2 newPos =
            alien.pos + md * alien.speed * 0.5f * deltaTime;
        if (!checkWallCollisionOnFloor(alien.floor, newPos, 20.0f)) {
          alien.pos = newPos;
        } else {
          alien.state = Alien::State::Idle;
          alien.idleTimer = 1.0f;
        }
        alien.rotation = std::atan2(md.y, md.x) - PI * 0.5f;
      }
      if (canSeePlayer) {
        alien.state = Alien::State::Chase;
      }
      break;
    }

    case Alien::State::Chase: {
      if (!canSeePlayer && distToPlayer > alien.sightRange * 1.3f) {
        alien.state = Alien::State::Idle;
        alien.idleTimer = 0.5f;
        break;
      }

      float chaseSpeed =
          m_rabbitPickedUp ? alien.speed * 1.3f : alien.speed;

      if (distToPlayer > alien.attackRange) {
        Vector2 md = toPlayer.normalized();
        Vector2 newPos =
            alien.pos + md * chaseSpeed * deltaTime;
        if (!checkWallCollisionOnFloor(alien.floor, newPos, 20.0f)) {
          alien.pos = newPos;
        }
        alien.rotation =
            std::atan2(toPlayer.y, toPlayer.x) - PI * 0.5f;
      } else {
        alien.state = Alien::State::Attack;
      }
      break;
    }

    case Alien::State::Attack: {
      if (distToPlayer > alien.attackRange * 1.5f) {
        alien.state = Alien::State::Chase;
        break;
      }

      alien.rotation =
          std::atan2(toPlayer.y, toPlayer.x) - PI * 0.5f;

      if (alien.attackCooldown <= 0.0f) {
        m_playerHP -= alien.damage;
        alien.attackCooldown = 1.0f;
        m_damageFlashTimer = 0.3f;
        shakeCamera(8.0f, 0.15f);
        spawnParticles(m_playerPos, 8, 1.0f, 0.1f, 0.1f);

        if (m_playerHP <= 0.0f) {
          m_playerHP = 0.0f;
          m_gameState = GameState::Dead;
          std::cout << "\n💀 YOU DIED! The aliens got you...\n";
          std::cout << "   You survived " << static_cast<int>(m_gameTime)
                    << " seconds and stomped " << m_killCount << " aliens.\n";
          std::cout << "   Press R to try again!\n\n";
          spawnParticles(m_playerPos, 30, 0.8f, 0.0f, 0.0f);
        }
      }
      break;
    }
    }
  }
}

// ─────────────────────── particles ─────────────────────────

void Application::spawnParticles(const Vector2 &pos, int count, float r,
                                 float g, float b) {
  spawnParticlesEx(pos, count, r, g, b, 3.0f, 8.0f, 30.0f, 110.0f,
                   0.3f, 1.0f, 0.0f);
}

void Application::spawnParticlesEx(const Vector2 &pos, int count, float r,
                                   float g, float b, float sizeMin,
                                   float sizeMax, float speedMin,
                                   float speedMax, float lifeMin,
                                   float lifeMax, float gravity) {
  for (int i = 0; i < count; ++i) {
    Particle p;
    p.pos = pos;
    float angle = m_randomDist(m_randomEngine) * PI;
    float spd = speedMin + std::abs(m_randomDist(m_randomEngine)) * (speedMax - speedMin);
    p.vel = {std::cos(angle) * spd, std::sin(angle) * spd};
    p.maxLife = lifeMin + std::abs(m_randomDist(m_randomEngine)) * (lifeMax - lifeMin);
    p.life = p.maxLife;
    p.r = std::clamp(r + m_randomDist(m_randomEngine) * 0.15f, 0.0f, 1.0f);
    p.g = std::clamp(g + m_randomDist(m_randomEngine) * 0.1f, 0.0f, 1.0f);
    p.b = std::clamp(b + m_randomDist(m_randomEngine) * 0.1f, 0.0f, 1.0f);
    p.a = 1.0f;
    p.size = sizeMin + std::abs(m_randomDist(m_randomEngine)) * (sizeMax - sizeMin);
    p.gravity = gravity;
    p.rotation = m_randomDist(m_randomEngine) * PI;
    p.rotSpeed = m_randomDist(m_randomEngine) * 5.0f;
    m_particles.push_back(p);
  }
}

void Application::updateParticles(float deltaTime) {
  for (auto &p : m_particles) {
    p.life -= deltaTime;
    p.pos += p.vel * deltaTime;
    p.vel.y -= p.gravity * deltaTime;  // gravity pulls down
    p.vel = p.vel * 0.96f;
    p.a = std::max(0.0f, p.life / p.maxLife);
    p.size *= 0.98f;
    p.rotation += p.rotSpeed * deltaTime;
  }
  m_particles.erase(
      std::remove_if(m_particles.begin(), m_particles.end(),
                     [](const Particle &p) { return p.life <= 0.0f; }),
      m_particles.end());
}

void Application::renderParticles() {
  for (const auto &p : m_particles) {
    m_renderer->drawQuadAlpha(p.pos, {p.size, p.size}, p.r, p.g, p.b, p.a,
                              p.rotation);
  }
}

// ─────────────────────── update ────────────────────────────

void Application::update(float deltaTime) {
  if (m_gameState != GameState::Playing) {
    m_winTimer += deltaTime;
    return;
  }

  m_gameTime += deltaTime;

  m_cameraPos.x += (m_playerPos.x - m_cameraPos.x) * m_cameraSmooth;
  m_cameraPos.y += (m_playerPos.y - m_cameraPos.y) * m_cameraSmooth;

  if (m_cameraShakeDuration > 0.0f) {
    m_cameraShakeTimer += deltaTime;
    float progress =
        std::min(m_cameraShakeTimer / m_cameraShakeDuration, 1.0f);
    m_cameraShakeIntensity *= (1.0f - progress);
    if (progress >= 1.0f) {
      m_cameraShakeDuration = 0.0f;
      m_cameraShakeIntensity = 0.0f;
      m_cameraShakeTimer = 0.0f;
    }
  }

  // Damage flash
  if (m_damageFlashTimer > 0.0f)
    m_damageFlashTimer -= deltaTime;

  // Rabbit bobbing animation
  m_rabbitBob += deltaTime * 3.0f;

  // Heartbeat effect when HP is low
  if (m_playerHP < 30.0f && m_playerHP > 0.0f) {
    m_heartbeatTimer += deltaTime;
    float intensity = (30.0f - m_playerHP) / 30.0f;
    if (std::fmod(m_heartbeatTimer, 1.0f) < 0.1f) {
      shakeCamera(2.0f * intensity, 0.05f);
    }
  }

  updateAliens(deltaTime);
  updateParticles(deltaTime);
  updateHints(deltaTime);

  // ── Contextual hints / tips ──
  m_hintCooldown -= deltaTime;

  // Start of game hint
  if (!m_shownStartHint && m_gameTime > 1.0f) {
    m_shownStartHint = true;
    showHint(5.0f, 0.3f, 0.9f, 1.0f, 0); // info
    std::cout << "💡 TIP: Find the rabbit on Floor 3 and bring it to the door "
                 "on Floor 1!\n";
    std::cout << "    Use WASD to move, Mouse to aim, SPACE to attack!\n";
  }

  // Floor change hint
  if (mCurrentFloor != m_lastHintFloor && m_hintCooldown <= 0.0f) {
    m_lastHintFloor = mCurrentFloor;
    m_hintCooldown = 3.0f;
    if (mCurrentFloor == 0) {
      showHint(3.0f, 0.3f, 1.0f, 0.4f, 2);  // objective
      if (!m_hasRabbit && !m_shownDoorHint) {
        m_shownDoorHint = true;
        std::cout << "🚪 The exit door is on this floor... but it's locked "
                     "without the rabbit!\n";
      } else if (m_hasRabbit) {
        std::cout << "🚪 Quick! The door is here! Run to it!\n";
      }
    } else if (mCurrentFloor == 2) {
      showHint(3.0f, 1.0f, 1.0f, 0.5f, 2);
      if (!m_hasRabbit && !m_shownRabbitHint) {
        m_shownRabbitHint = true;
        std::cout << "🐇 The rabbit is somewhere on this floor! Look for the "
                     "white glow!\n";
      }
    }
  }

  // Attack hint when aliens are close
  if (!m_shownAttackHint) {
    for (const auto &alien : m_aliens) {
      if (alien.alive && alien.floor == mCurrentFloor &&
          alien.state != Alien::State::Dying) {
        float dist = Vector2::distance(m_playerPos, alien.pos);
        if (dist < 200.0f) {
          m_shownAttackHint = true;
          showHint(3.0f, 1.0f, 0.4f, 0.3f, 1);  // warning
          std::cout
              << "⚠️  Alien nearby! Press SPACE to stomp it!\n";
          break;
        }
      }
    }
  }

  // Periodic funny tips
  if (m_gameTime > 0.0f && std::fmod(m_gameTime, 45.0f) < deltaTime &&
      m_hintCooldown <= 0.0f) {
    int tip = static_cast<int>(m_gameTime / 45.0f) % 6;
    const char *tips[] = {
        "🎮 Pro tip: Running makes you faster. Shocking, right?",
        "🤔 Fun fact: These aliens came here for the rabbit too. Weirdos.",
        "🏃 Don't forget to sprint (Shift)! Your legs are not decoration!",
        "👀 The aliens can see you... but only if you let them.",
        "🐇 That rabbit better be worth it...",
        "💀 Remember: dying is just a learning experience!",
    };
    std::cout << tips[tip] << "\n";
    showHint(3.0f, 0.9f, 0.8f, 0.3f, 3);  // funny
    m_hintCooldown = 10.0f;
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

  double mx, my;
  glfwGetCursorPos(m_window, &mx, &my);
  float wmx = m_cameraPos.x + (static_cast<float>(mx) - mWindowWidth / 2.0f);
  float wmy = m_cameraPos.y + (mWindowHeight / 2.0f - static_cast<float>(my));
  auto [gx, gy] = map.worldToGrid({wmx, wmy});

  if (gx >= 0 && gx < map.getWidth() && gy >= 0 && gy < map.getHeight()) {
    Vector2 tp = map.gridToWorld(gx, gy);
    float half = ts / 2.0f;
    float bw = 3.0f;
    m_renderer->drawQuad({tp.x, tp.y + half - bw / 2.0f}, {ts, bw}, 1.0f,
                         1.0f, 0.0f, 0.0f);
    m_renderer->drawQuad({tp.x, tp.y - half + bw / 2.0f}, {ts, bw}, 1.0f,
                         1.0f, 0.0f, 0.0f);
    m_renderer->drawQuad({tp.x - half + bw / 2.0f, tp.y}, {bw, ts}, 1.0f,
                         1.0f, 0.0f, 0.0f);
    m_renderer->drawQuad({tp.x + half - bw / 2.0f, tp.y}, {bw, ts}, 1.0f,
                         1.0f, 0.0f, 0.0f);
  }

  m_renderer->beginScreenSpace();

  const auto &defs = map.getTileDefs();
  float palX = 20.0f;
  float palY = 20.0f;
  float boxSize = 32.0f;
  float gap = 6.0f;

  float barW = defs.size() * (boxSize + gap) + gap;
  m_renderer->drawQuad(
      {palX + barW / 2.0f - gap / 2.0f, palY + boxSize / 2.0f},
      {barW, boxSize + 12.0f}, 0.08f, 0.08f, 0.12f, 0.0f);

  for (int i = 0; i < static_cast<int>(defs.size()); ++i) {
    float bx = palX + i * (boxSize + gap) + boxSize / 2.0f;
    float by = palY + boxSize / 2.0f;

    if (i == m_editorTileIndex) {
      m_renderer->drawQuad({bx, by}, {boxSize + 6.0f, boxSize + 6.0f}, 1.0f,
                           1.0f, 0.0f, 0.0f);
    }

    if (defs[i].symbol == ' ') {
      m_renderer->drawQuad({bx, by}, {boxSize, boxSize}, 0.15f, 0.15f, 0.2f,
                           0.0f);
    } else {
      m_renderer->drawQuad({bx, by}, {boxSize, boxSize}, defs[i].r, defs[i].g,
                           defs[i].b, 0.0f);
    }
  }

  m_renderer->drawQuad({static_cast<float>(mWindowWidth) - 60.0f,
                         static_cast<float>(mWindowHeight) - 20.0f},
                        {100, 28}, 0.9f, 0.2f, 0.2f, 0.0f);

  Vector2 shake{0.0f, 0.0f};
  m_renderer->setCameraPosition(m_cameraPos, shake);
}

// ─────────────────────── render aliens ─────────────────────

void Application::renderAliens() {
  for (const auto &alien : m_aliens) {
    if (!alien.alive && alien.state != Alien::State::Dying)
      continue;
    if (alien.floor != mCurrentFloor)
      continue;

    // === DYING ALIEN — squashed with blood pool ===
    if (alien.state == Alien::State::Dying) {
      float t = alien.deathTimer / alien.deathDuration;

      // Blood pool expanding
      float poolSize = 30.0f + t * 100.0f;
      float poolAlpha = 0.8f - t * 0.4f;
      m_renderer->drawQuadAlpha(alien.pos, {poolSize, poolSize * 0.6f},
                                0.5f, 0.0f, 0.0f, poolAlpha, 0.0f);
      m_renderer->drawQuadAlpha(alien.pos, {poolSize * 0.7f, poolSize * 0.5f},
                                0.7f, 0.05f, 0.05f, poolAlpha * 0.8f,
                                alien.rotation * 0.5f);

      // Squashed body
      float sq = alien.squash;
      if (sq > 0.05f) {
        float bodyW = 48.0f * (1.0f + (1.0f - sq) * 0.8f); // wider as squashed
        float bodyH = 48.0f * sq;                            // shorter
        m_renderer->drawQuadAlpha(alien.pos, {bodyW, bodyH},
                                  0.1f, 0.4f * sq, 0.1f, sq,
                                  alien.rotation);
      }
      continue;
    }

    // === ALIVE ALIEN ===
    float wobble = std::sin(alien.animTimer * 4.0f) * 2.0f;

    float bodyR = 0.15f, bodyG = 0.75f, bodyB = 0.15f;

    if (alien.state == Alien::State::Chase ||
        alien.state == Alien::State::Attack) {
      float pulse = (std::sin(alien.animTimer * 8.0f) + 1.0f) * 0.5f;
      bodyR = 0.4f + pulse * 0.4f;
      bodyG = 0.3f + pulse * 0.2f;
      bodyB = 0.1f;
    }

    // Main body
    m_renderer->drawQuad({alien.pos.x, alien.pos.y + wobble}, {48, 48}, bodyR,
                         bodyG, bodyB, alien.rotation);

    // Head
    float headAngle = alien.rotation + PI * 0.5f;
    Vector2 headOffset{std::cos(headAngle) * 20.0f,
                       std::sin(headAngle) * 20.0f};
    m_renderer->drawQuad(
        {alien.pos.x + headOffset.x, alien.pos.y + headOffset.y + wobble},
        {24, 24}, bodyR * 0.8f, bodyG * 1.2f, bodyB * 0.8f, alien.rotation);

    // Eyes
    float eyeSpread = 8.0f;
    float perpAngle = headAngle + PI * 0.5f;
    Vector2 eyeBase = alien.pos + headOffset;
    for (int e = -1; e <= 1; e += 2) {
      Vector2 eyePos = {
          eyeBase.x + std::cos(perpAngle) * eyeSpread * e,
          eyeBase.y + std::sin(perpAngle) * eyeSpread * e + wobble};
      m_renderer->drawQuad(eyePos, {6, 6}, 1.0f, 0.0f, 0.0f, 0.0f);
    }

    // Alert indicator when chasing
    if (alien.state == Alien::State::Chase ||
        alien.state == Alien::State::Attack) {
      float alertBob = std::sin(alien.animTimer * 6.0f) * 5.0f;
      m_renderer->drawQuad(
          {alien.pos.x, alien.pos.y + 45.0f + alertBob}, {8, 20}, 1.0f, 0.3f,
          0.3f, 0.0f);
      m_renderer->drawQuad(
          {alien.pos.x, alien.pos.y + 30.0f + alertBob}, {6, 6}, 1.0f, 0.3f,
          0.3f, 0.0f);
    }

    // HP bar (only show if damaged)
    if (alien.hp < alien.maxHp) {
      float hpFrac = alien.hp / alien.maxHp;
      float barW = 40.0f;
      float barH = 5.0f;
      float barY = alien.pos.y + 55.0f;
      m_renderer->drawQuadAlpha({alien.pos.x, barY}, {barW + 2, barH + 2},
                                0.0f, 0.0f, 0.0f, 0.6f);
      float fillW = barW * hpFrac;
      float hpR = hpFrac < 0.5f ? 1.0f : 1.0f - (hpFrac - 0.5f) * 2.0f;
      float hpG = hpFrac > 0.5f ? 1.0f : hpFrac * 2.0f;
      m_renderer->drawQuad({alien.pos.x - (barW - fillW) / 2.0f, barY},
                           {fillW, barH}, hpR, hpG, 0.1f, 0.0f);
    }
  }
}

// ─────────────────────── HUD ──────────────────────────────

void Application::renderHUD() {
  m_renderer->beginScreenSpace();

  float hudX = 20.0f;
  float hudY = static_cast<float>(mWindowHeight) - 30.0f;

  // ── HP Bar ──
  float hpBarW = 200.0f;
  float hpBarH = 18.0f;
  float hpFrac = m_playerHP / m_playerMaxHP;

  m_renderer->drawQuadAlpha({hudX + hpBarW / 2.0f, hudY},
                            {hpBarW + 4, hpBarH + 4}, 0.0f, 0.0f, 0.0f, 0.8f);
  float hpR = hpFrac < 0.5f ? 1.0f : 1.0f - (hpFrac - 0.5f) * 2.0f;
  float hpG = hpFrac > 0.5f ? 1.0f : hpFrac * 2.0f;
  float fillW = hpBarW * hpFrac;
  if (fillW > 0.1f) {
    m_renderer->drawQuad({hudX + fillW / 2.0f, hudY}, {fillW, hpBarH}, hpR,
                         hpG, 0.1f, 0.0f);
  }
  m_renderer->drawQuadAlpha({hudX + hpBarW / 2.0f, hudY},
                            {hpBarW + 2, hpBarH + 2}, 0.8f, 0.8f, 0.8f, 0.3f);

  // HP label (red square)
  m_renderer->drawQuad({hudX - 8.0f, hudY}, {12, 12}, 1.0f, 0.3f, 0.3f,
                       0.0f);

  // ── Stamina Bar ──
  float stamY = hudY - 25.0f;
  float stamBarW = 160.0f;
  float stamBarH = 12.0f;
  float stamFrac = m_playerStamina / m_playerMaxStamina;

  m_renderer->drawQuadAlpha({hudX + stamBarW / 2.0f, stamY},
                            {stamBarW + 4, stamBarH + 4}, 0.0f, 0.0f, 0.0f,
                            0.8f);
  float stamFillW = stamBarW * stamFrac;
  if (stamFillW > 0.1f) {
    float stamPulse =
        m_isSprinting ? (std::sin(m_gameTime * 10.0f) * 0.1f + 0.9f) : 1.0f;
    m_renderer->drawQuad({hudX + stamFillW / 2.0f, stamY},
                         {stamFillW, stamBarH}, 0.2f * stamPulse,
                         0.6f * stamPulse, 1.0f * stamPulse, 0.0f);
  }
  m_renderer->drawQuadAlpha({hudX + stamBarW / 2.0f, stamY},
                            {stamBarW + 2, stamBarH + 2}, 0.5f, 0.7f, 1.0f,
                            0.3f);

  // Sprint indicator (two chevrons)
  if (m_isSprinting) {
    m_renderer->drawQuad({hudX + stamBarW + 20.0f, stamY}, {8, 14}, 0.3f,
                         0.7f, 1.0f, 0.0f);
    m_renderer->drawQuad({hudX + stamBarW + 30.0f, stamY}, {8, 14}, 0.3f,
                         0.7f, 1.0f, 0.0f);
  }

  // ── Floor indicator ──
  float floorX = static_cast<float>(mWindowWidth) - 120.0f;
  float floorY = static_cast<float>(mWindowHeight) - 30.0f;
  m_renderer->drawQuadAlpha({floorX, floorY}, {100, 30}, 0.05f, 0.05f, 0.1f,
                            0.8f);

  const float floorColors[3][3] = {
      {0.4f, 0.2f, 0.6f}, {0.3f, 0.5f, 0.2f}, {0.6f, 0.2f, 0.2f}};

  for (int i = 0; i < 3; ++i) {
    float labelX = floorX - 30.0f + i * 25.0f;
    if (i == mCurrentFloor) {
      m_renderer->drawQuad({labelX, floorY}, {20, 22},
                           floorColors[i][0] * 2.0f, floorColors[i][1] * 2.0f,
                           floorColors[i][2] * 2.0f, 0.0f);
    } else {
      m_renderer->drawQuad({labelX, floorY}, {16, 16}, floorColors[i][0],
                           floorColors[i][1], floorColors[i][2], 0.0f);
    }
  }

  // ── Objective status ──
  float objX = static_cast<float>(mWindowWidth) / 2.0f;
  float objY = static_cast<float>(mWindowHeight) - 25.0f;

  if (!m_hasRabbit) {
    m_renderer->drawQuadAlpha({objX, objY}, {280, 24}, 0.0f, 0.0f, 0.0f,
                              0.6f);
    m_renderer->drawQuad({objX - 120.0f, objY}, {16, 16}, 1.0f, 1.0f, 1.0f,
                         0.0f);
    m_renderer->drawQuad({objX + 120.0f, objY}, {10, 16}, 0.5f, 0.8f, 0.5f,
                         0.0f);
  } else {
    float pulse = (std::sin(m_gameTime * 4.0f) + 1.0f) * 0.5f;
    m_renderer->drawQuadAlpha({objX, objY}, {250, 24}, 0.1f * pulse, 0.3f,
                              0.05f, 0.7f);
    m_renderer->drawQuad({objX - 110.0f, objY}, {16, 16}, 1.0f, 1.0f, 1.0f,
                         0.0f);
    m_renderer->drawQuad({objX + 110.0f, objY}, {10, 16}, 1.0f, 0.5f, 0.3f,
                         PI);
  }

  // ── Timer & Kill count ──
  float timerY = static_cast<float>(mWindowHeight) - 55.0f;
  m_renderer->drawQuadAlpha({objX, timerY}, {80, 18}, 0.0f, 0.0f, 0.0f,
                            0.5f);
  int minutes = static_cast<int>(m_gameTime) / 60;
  int seconds = static_cast<int>(m_gameTime) % 60;
  for (int i = 0; i < std::min(minutes, 8); ++i) {
    m_renderer->drawQuad({objX - 30.0f + i * 8.0f, timerY}, {5, 10}, 0.9f,
                         0.7f, 0.2f, 0.0f);
  }
  float secFrac = static_cast<float>(seconds) / 60.0f;
  m_renderer->drawQuad({objX + 10.0f, timerY}, {30.0f * secFrac, 6}, 0.7f,
                       0.7f, 0.7f, 0.0f);

  // Kill count (skull icons)
  if (m_killCount > 0) {
    float killY = timerY - 22.0f;
    m_renderer->drawQuadAlpha({objX, killY}, {100, 18}, 0.0f, 0.0f, 0.0f,
                              0.4f);
    // Skull icon
    m_renderer->drawQuad({objX - 40.0f, killY}, {10, 10}, 0.9f, 0.9f, 0.9f,
                         0.0f);
    m_renderer->drawQuad({objX - 43.0f, killY + 1.0f}, {3, 3}, 0.1f, 0.1f,
                         0.1f, 0.0f);
    m_renderer->drawQuad({objX - 37.0f, killY + 1.0f}, {3, 3}, 0.1f, 0.1f,
                         0.1f, 0.0f);
    // Kill count bar
    for (int k = 0; k < std::min(m_killCount, 10); ++k) {
      m_renderer->drawQuad({objX - 25.0f + k * 8.0f, killY}, {5, 12}, 0.8f,
                           0.2f, 0.2f, 0.0f);
    }
  }

  // ── Damage flash overlay ──
  if (m_damageFlashTimer > 0.0f) {
    float alpha = m_damageFlashTimer / 0.3f * 0.4f;
    m_renderer->drawQuadAlpha(
        {static_cast<float>(mWindowWidth) / 2.0f,
         static_cast<float>(mWindowHeight) / 2.0f},
        {static_cast<float>(mWindowWidth), static_cast<float>(mWindowHeight)},
        0.8f, 0.0f, 0.0f, alpha);
  }

  // ── Low HP vignette ──
  if (m_playerHP < 30.0f && m_playerHP > 0.0f) {
    float intensity = (30.0f - m_playerHP) / 30.0f;
    float pulse = (std::sin(m_heartbeatTimer * 4.0f) + 1.0f) * 0.5f;
    float alpha = intensity * 0.3f * pulse;
    m_renderer->drawQuadAlpha(
        {static_cast<float>(mWindowWidth) / 2.0f,
         static_cast<float>(mWindowHeight) / 2.0f},
        {static_cast<float>(mWindowWidth), static_cast<float>(mWindowHeight)},
        0.5f, 0.0f, 0.0f, alpha);
  }
}

// ─────────────────────── game over / win screen ───────────

void Application::renderGameOver() {
  m_renderer->beginScreenSpace();

  float cx = static_cast<float>(mWindowWidth) / 2.0f;
  float cy = static_cast<float>(mWindowHeight) / 2.0f;

  // Dark overlay
  m_renderer->drawQuadAlpha(
      {cx, cy},
      {static_cast<float>(mWindowWidth), static_cast<float>(mWindowHeight)},
      0.0f, 0.0f, 0.0f, 0.7f);

  if (m_gameState == GameState::Won) {
    float pulse = (std::sin(m_winTimer * 3.0f) + 1.0f) * 0.5f;
    m_renderer->drawQuad({cx, cy + 40.0f}, {300, 50}, 0.1f + pulse * 0.2f,
                         0.5f + pulse * 0.3f, 0.1f + pulse * 0.1f, 0.0f);
    // Rabbit icon
    m_renderer->drawQuad({cx, cy - 20.0f}, {40, 40}, 1.0f, 1.0f, 1.0f, 0.0f);
    // Stars
    for (int i = 0; i < 5; ++i) {
      float angle = m_winTimer * 2.0f + i * PI * 2.0f / 5.0f;
      float dist = 80.0f + std::sin(m_winTimer * 1.5f + i) * 20.0f;
      m_renderer->drawQuad(
          {cx + std::cos(angle) * dist, cy + std::sin(angle) * dist}, {10, 10},
          1.0f, 0.9f, 0.2f, angle);
    }
  } else {
    // Death screen
    float fade = std::min(m_winTimer, 1.0f);
    m_renderer->drawQuadAlpha({cx, cy + 30.0f}, {250 * fade, 40 * fade}, 0.7f,
                              0.1f, 0.1f, 0.9f);
    // Skull shape
    m_renderer->drawQuad({cx, cy - 20.0f}, {30, 30}, 0.9f, 0.9f, 0.9f, 0.0f);
    m_renderer->drawQuad({cx - 8.0f, cy - 15.0f}, {6, 8}, 0.1f, 0.1f, 0.1f,
                         0.0f);
    m_renderer->drawQuad({cx + 8.0f, cy - 15.0f}, {6, 8}, 0.1f, 0.1f, 0.1f,
                         0.0f);
  }

  // "Press R" hint
  float hintPulse = (std::sin(m_winTimer * 2.0f) + 1.0f) * 0.5f;
  m_renderer->drawQuadAlpha({cx, cy - 80.0f}, {200, 20}, 0.3f, 0.3f, 0.3f,
                            0.5f + hintPulse * 0.3f);
  m_renderer->drawQuad({cx - 60.0f, cy - 80.0f}, {15, 12}, 1.0f, 1.0f, 1.0f,
                       0.0f);
}

// ─────────────────────── minimap ──────────────────────────

void Application::renderMinimap() {
  if (mCurrentFloor < 0 || mCurrentFloor >= static_cast<int>(m_floors.size()))
    return;

  m_renderer->beginScreenSpace();

  float mapScale = 3.0f;
  float mmX = static_cast<float>(mWindowWidth) - 10.0f;
  float mmY = 10.0f;

  const auto &map = m_floors[mCurrentFloor];
  float mmW = map.getWidth() * mapScale;
  float mmH = map.getHeight() * mapScale;

  mmX -= mmW;

  // Background
  m_renderer->drawQuadAlpha({mmX + mmW / 2.0f, mmY + mmH / 2.0f},
                            {mmW + 4, mmH + 4}, 0.0f, 0.0f, 0.0f, 0.6f);

  for (int gy = 0; gy < map.getHeight(); ++gy) {
    for (int gx = 0; gx < map.getWidth(); ++gx) {
      char c = map.getTile(gx, gy);
      if (c == ' ')
        continue;

      const TileDef *def = map.getDefForSymbol(c);
      if (!def)
        continue;

      float px = mmX + gx * mapScale + mapScale / 2.0f;
      float py =
          mmY + (map.getHeight() - 1 - gy) * mapScale + mapScale / 2.0f;

      if (def->solid) {
        m_renderer->drawQuadAlpha({px, py}, {mapScale, mapScale}, def->r,
                                  def->g, def->b, 0.7f);
      } else if (c == 'E' || c == 'S') {
        m_renderer->drawQuadAlpha({px, py}, {mapScale, mapScale}, 0.8f, 0.5f,
                                  0.2f, 0.8f);
      }
    }
  }

  // Player dot
  float playerMMX = mmX + (m_playerPos.x / map.getTileSize()) * mapScale;
  float playerMMY =
      mmY + (map.getHeight() - m_playerPos.y / map.getTileSize()) * mapScale;
  float blink = (std::sin(m_gameTime * 6.0f) + 1.0f) * 0.5f;
  m_renderer->drawQuad({playerMMX, playerMMY},
                       {mapScale * 2.0f, mapScale * 2.0f}, 0.2f + blink * 0.8f,
                       0.8f, 0.2f, m_playerRotation);

  // Alien dots
  for (const auto &alien : m_aliens) {
    if (!alien.alive || alien.floor != mCurrentFloor)
      continue;
    float ax = mmX + (alien.pos.x / map.getTileSize()) * mapScale;
    float ay =
        mmY + (map.getHeight() - alien.pos.y / map.getTileSize()) * mapScale;
    float alertColor = (alien.state == Alien::State::Chase ||
                        alien.state == Alien::State::Attack)
                           ? 1.0f
                           : 0.5f;
    m_renderer->drawQuad({ax, ay}, {mapScale * 1.5f, mapScale * 1.5f},
                         alertColor, 0.2f, 0.2f, 0.0f);
  }

  // Rabbit on minimap
  if (!m_hasRabbit && mCurrentFloor == m_rabbitFloor) {
    float rx = mmX + (m_rabbitPos.x / map.getTileSize()) * mapScale;
    float ry = mmY + (map.getHeight() - m_rabbitPos.y / map.getTileSize()) *
                         mapScale;
    float pulse = (std::sin(m_gameTime * 4.0f) + 1.0f) * 0.5f;
    m_renderer->drawQuad({rx, ry}, {mapScale * 2.0f, mapScale * 2.0f}, 1.0f,
                         1.0f, 0.5f + pulse * 0.5f, 0.0f);
  }
}

// ─────────────────────── hints / messages ─────────────────

void Application::showHint(float duration, float r, float g, float b,
                           int icon) {
  HintMessage h;
  h.timer = duration;
  h.duration = duration;
  h.r = r;
  h.g = g;
  h.b = b;
  h.iconType = icon;
  m_hints.push_back(h);
}

void Application::updateHints(float deltaTime) {
  for (auto &h : m_hints) {
    h.timer -= deltaTime;
  }
  m_hints.erase(
      std::remove_if(m_hints.begin(), m_hints.end(),
                     [](const HintMessage &h) { return h.timer <= 0.0f; }),
      m_hints.end());
}

void Application::renderHints() {
  m_renderer->beginScreenSpace();

  float hintX = static_cast<float>(mWindowWidth) / 2.0f;
  float baseY = 80.0f;

  for (int i = 0; i < static_cast<int>(m_hints.size()); ++i) {
    const auto &h = m_hints[i];
    float alpha = std::min(h.timer, 1.0f); // fade out last second
    if (h.timer > h.duration - 0.3f)
      alpha = (h.duration - h.timer) / 0.3f; // fade in

    float y = baseY + i * 32.0f;

    // Background bar
    m_renderer->drawQuadAlpha({hintX, y}, {320, 26}, 0.05f, 0.05f, 0.1f,
                              0.7f * alpha);

    // Left icon based on type
    float iconX = hintX - 145.0f;
    switch (h.iconType) {
    case 0: // Info — circle "i"
      m_renderer->drawQuadAlpha({iconX, y}, {18, 18}, 0.3f, 0.7f, 1.0f,
                                alpha, 0.0f);
      m_renderer->drawQuadAlpha({iconX, y + 2.0f}, {4, 8}, 1.0f, 1.0f, 1.0f,
                                alpha, 0.0f);
      m_renderer->drawQuadAlpha({iconX, y + 7.0f}, {4, 4}, 1.0f, 1.0f, 1.0f,
                                alpha, 0.0f);
      break;
    case 1: // Warning — triangle "!"
      m_renderer->drawQuadAlpha({iconX, y}, {20, 18}, 1.0f, 0.7f, 0.1f,
                                alpha, 0.0f);
      m_renderer->drawQuadAlpha({iconX, y + 1.0f}, {3, 8}, 0.1f, 0.1f, 0.0f,
                                alpha, 0.0f);
      m_renderer->drawQuadAlpha({iconX, y - 5.0f}, {3, 3}, 0.1f, 0.1f, 0.0f,
                                alpha, 0.0f);
      break;
    case 2: // Objective — star
      m_renderer->drawQuadAlpha({iconX, y}, {14, 14}, h.r, h.g, h.b, alpha,
                                0.78f);
      m_renderer->drawQuadAlpha({iconX, y}, {14, 14}, h.r, h.g, h.b, alpha,
                                0.0f);
      break;
    case 3: // Funny — smiley
      m_renderer->drawQuadAlpha({iconX, y}, {18, 18}, 0.9f, 0.8f, 0.2f,
                                alpha, 0.0f);
      m_renderer->drawQuadAlpha({iconX - 4.0f, y + 3.0f}, {3, 3}, 0.1f, 0.1f,
                                0.0f, alpha, 0.0f);
      m_renderer->drawQuadAlpha({iconX + 4.0f, y + 3.0f}, {3, 3}, 0.1f, 0.1f,
                                0.0f, alpha, 0.0f);
      m_renderer->drawQuadAlpha({iconX, y - 3.0f}, {8, 3}, 0.1f, 0.1f, 0.0f,
                                alpha, 0.0f);
      break;
    }

    // Colored accent bar
    m_renderer->drawQuadAlpha({hintX, y - 12.0f}, {300, 2}, h.r, h.g, h.b,
                              0.8f * alpha);
  }
}

void Application::renderWaypoint() {
  // Show a directional arrow pointing toward the current objective
  m_renderer->beginScreenSpace();

  Vector2 target;
  float targetR, targetG, targetB;
  bool showWaypoint = false;

  if (!m_hasRabbit) {
    // Point to rabbit (if on same floor) or to stairs/elevator
    if (mCurrentFloor == m_rabbitFloor) {
      target = m_rabbitPos;
      targetR = 1.0f;
      targetG = 1.0f;
      targetB = 0.8f;
      showWaypoint = true;
    } else {
      // Point to nearest elevator/stair to go up
      if (mCurrentFloor < m_rabbitFloor &&
          mCurrentFloor < static_cast<int>(m_floors.size())) {
        auto stairs = m_floors[mCurrentFloor].findTiles('S');
        auto elevs = m_floors[mCurrentFloor].findTiles('E');
        float bestDist = 999999.0f;
        for (const auto &s : stairs) {
          float d = Vector2::distanceSq(m_playerPos, s);
          if (d < bestDist) {
            bestDist = d;
            target = s;
          }
        }
        for (const auto &e : elevs) {
          float d = Vector2::distanceSq(m_playerPos, e);
          if (d < bestDist) {
            bestDist = d;
            target = e;
          }
        }
        if (bestDist < 999999.0f) {
          targetR = 0.5f;
          targetG = 0.8f;
          targetB = 1.0f;
          showWaypoint = true;
        }
      }
    }
  } else {
    // Point to exit door
    if (mCurrentFloor == m_exitFloor) {
      target = m_exitPos;
      targetR = 0.3f;
      targetG = 1.0f;
      targetB = 0.4f;
      showWaypoint = true;
    } else {
      // Point to stairs/elevator to go down
      if (mCurrentFloor > m_exitFloor &&
          mCurrentFloor < static_cast<int>(m_floors.size())) {
        auto stairs = m_floors[mCurrentFloor].findTiles('S');
        auto elevs = m_floors[mCurrentFloor].findTiles('E');
        float bestDist = 999999.0f;
        for (const auto &s : stairs) {
          float d = Vector2::distanceSq(m_playerPos, s);
          if (d < bestDist) {
            bestDist = d;
            target = s;
          }
        }
        for (const auto &e : elevs) {
          float d = Vector2::distanceSq(m_playerPos, e);
          if (d < bestDist) {
            bestDist = d;
            target = e;
          }
        }
        if (bestDist < 999999.0f) {
          targetR = 1.0f;
          targetG = 0.5f;
          targetB = 0.3f;
          showWaypoint = true;
        }
      }
    }
  }

  if (!showWaypoint)
    return;

  // Calculate screen-space direction to target
  float screenTargetX =
      (target.x - m_cameraPos.x) + (mWindowWidth * 0.5f);
  float screenTargetY =
      (target.y - m_cameraPos.y) + (mWindowHeight * 0.5f);

  float cx = mWindowWidth * 0.5f;
  float cy = mWindowHeight * 0.5f;
  float dirX = screenTargetX - cx;
  float dirY = screenTargetY - cy;
  float dist = std::sqrt(dirX * dirX + dirY * dirY);

  // Only show if target is off-screen or far
  if (dist < 100.0f)
    return;

  float angle = std::atan2(dirY, dirX);
  float edgeDist = 60.0f; // distance from screen edge

  // Clamp arrow to screen edge
  float arrowX = cx + std::cos(angle) * std::min(dist, cx - edgeDist);
  float arrowY = cy + std::sin(angle) * std::min(dist, cy - edgeDist);

  float pulse = (std::sin(m_gameTime * 4.0f) + 1.0f) * 0.5f;
  float arrowAlpha = 0.5f + pulse * 0.3f;

  // Arrow triangle (using rotated quad)
  m_renderer->drawQuadAlpha({arrowX, arrowY}, {20, 12}, targetR, targetG,
                            targetB, arrowAlpha, angle);
  // Arrow stem
  m_renderer->drawQuadAlpha(
      {arrowX - std::cos(angle) * 12.0f, arrowY - std::sin(angle) * 12.0f},
      {12, 6}, targetR, targetG, targetB, arrowAlpha * 0.7f, angle);
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
  if (mCurrentFloor >= 0 &&
      mCurrentFloor < static_cast<int>(m_floors.size())) {
    m_floors[mCurrentFloor].render(*m_renderer);

    // ── Elevator tiles (E) ──
    auto elevPositions = m_floors[mCurrentFloor].findTiles('E');
    for (const auto &ep : elevPositions) {
      bool nearby = Vector2::distanceSq(m_playerPos, ep) < 150.0f * 150.0f;
      m_renderer->drawQuad(ep, {80, 80}, nearby ? 0.9f : 0.5f,
                           nearby ? 0.3f : 0.3f, nearby ? 0.4f : 0.7f, 0.0f);
    }

    // ── Stair tiles (S) ──
    auto stairPositions = m_floors[mCurrentFloor].findTiles('S');
    for (const auto &sp : stairPositions) {
      bool nearby = Vector2::distanceSq(m_playerPos, sp) < 150.0f * 150.0f;
      for (int i = 0; i < 3; ++i) {
        m_renderer->drawQuad({sp.x, sp.y - 20.0f + i * 15.0f}, {100, 8},
                             nearby ? 0.8f : 0.6f, nearby ? 0.6f : 0.4f,
                             nearby ? 0.2f : 0.3f, 0.0f);
      }
    }

    // ── Exit Door (X) — proper door on floor 0 ──
    if (mCurrentFloor == m_exitFloor) {
      auto exitTiles = m_floors[mCurrentFloor].findTiles('X');
      for (const auto &xp : exitTiles) {
        float pulse = (std::sin(m_gameTime * 3.0f) + 1.0f) * 0.5f;
        bool active = m_hasRabbit;

        // Door frame (dark border)
        m_renderer->drawQuad(xp, {96, 100}, 0.25f, 0.15f, 0.1f, 0.0f);
        // Door panel
        float doorR = active ? 0.3f + pulse * 0.2f : 0.4f;
        float doorG = active ? 0.6f + pulse * 0.3f : 0.3f;
        float doorB = active ? 0.2f : 0.25f;
        m_renderer->drawQuad(xp, {80, 92}, doorR, doorG, doorB, 0.0f);
        // Door panels (cross pattern)
        m_renderer->drawQuad({xp.x, xp.y + 20.0f}, {60, 30}, doorR * 0.85f,
                             doorG * 0.85f, doorB * 0.85f, 0.0f);
        m_renderer->drawQuad({xp.x, xp.y - 20.0f}, {60, 30}, doorR * 0.85f,
                             doorG * 0.85f, doorB * 0.85f, 0.0f);
        // Door handle
        m_renderer->drawQuad({xp.x + 28.0f, xp.y}, {8, 12},
                             0.8f, 0.7f, 0.3f, 0.0f);
        // Glow when active
        if (active) {
          m_renderer->drawQuadAlpha(xp, {120, 120}, 0.2f, 0.9f, 0.3f,
                                    0.15f + pulse * 0.1f, 0.0f);
          // Arrow pointing at door
          float arrowBob = std::sin(m_gameTime * 5.0f) * 10.0f;
          m_renderer->drawQuad(
              {xp.x, xp.y + 65.0f + arrowBob}, {16, 24}, 0.2f, 1.0f, 0.3f,
              PI);
        } else {
          // Locked indicator — red X on door
          m_renderer->drawQuadAlpha(xp, {50, 6}, 0.8f, 0.2f, 0.2f, 0.5f,
                                    0.78f);
          m_renderer->drawQuadAlpha(xp, {50, 6}, 0.8f, 0.2f, 0.2f, 0.5f,
                                    -0.78f);
        }
      }
    }

    // ── Rabbit (white block on floor 2) ──
    if (!m_hasRabbit && mCurrentFloor == m_rabbitFloor) {
      float bob = std::sin(m_rabbitBob) * 8.0f;
      float glow = (std::sin(m_rabbitBob * 1.5f) + 1.0f) * 0.5f;

      // Glow
      m_renderer->drawQuadAlpha({m_rabbitPos.x, m_rabbitPos.y + bob}, {70, 70},
                                1.0f, 1.0f, 0.8f + glow * 0.2f,
                                0.2f + glow * 0.1f, 0.0f);
      // Body
      m_renderer->drawQuad({m_rabbitPos.x, m_rabbitPos.y + bob}, {40, 40},
                           1.0f, 1.0f, 1.0f, 0.0f);
      // Ears
      m_renderer->drawQuad(
          {m_rabbitPos.x - 10.0f, m_rabbitPos.y + 25.0f + bob}, {8, 18}, 1.0f,
          0.9f, 0.9f, 0.1f);
      m_renderer->drawQuad(
          {m_rabbitPos.x + 10.0f, m_rabbitPos.y + 25.0f + bob}, {8, 18}, 1.0f,
          0.9f, 0.9f, -0.1f);
      // Eyes
      m_renderer->drawQuad(
          {m_rabbitPos.x - 8.0f, m_rabbitPos.y + 5.0f + bob}, {5, 5}, 0.2f,
          0.0f, 0.0f, 0.0f);
      m_renderer->drawQuad(
          {m_rabbitPos.x + 8.0f, m_rabbitPos.y + 5.0f + bob}, {5, 5}, 0.2f,
          0.0f, 0.0f, 0.0f);

      // "Press F" prompt
      float distToRabbit = Vector2::distance(m_playerPos, m_rabbitPos);
      if (distToRabbit < 120.0f) {
        float promptBob = std::sin(m_gameTime * 3.0f) * 5.0f;
        m_renderer->drawQuadAlpha(
            {m_rabbitPos.x, m_rabbitPos.y + 55.0f + promptBob}, {60, 20},
            0.0f, 0.0f, 0.0f, 0.7f, 0.0f);
        m_renderer->drawQuad(
            {m_rabbitPos.x, m_rabbitPos.y + 55.0f + promptBob}, {15, 12},
            1.0f, 1.0f, 1.0f, 0.0f);
      }
    }

    // ── Environment decorations ──
    // Crates (D)
    auto decors = m_floors[mCurrentFloor].findTiles('D');
    for (const auto &dp : decors) {
      m_renderer->drawQuad(dp, {60, 60}, 0.55f, 0.35f, 0.15f, 0.0f);
      m_renderer->drawQuad(dp, {50, 50}, 0.65f, 0.45f, 0.2f, 0.0f);
      m_renderer->drawQuad(dp, {50, 6}, 0.45f, 0.3f, 0.1f, 0.0f);
      m_renderer->drawQuad(dp, {6, 50}, 0.45f, 0.3f, 0.1f, 0.0f);
    }

    // Lamps (L)
    auto lamps = m_floors[mCurrentFloor].findTiles('L');
    for (const auto &lp : lamps) {
      float flicker = 0.8f + std::sin(m_gameTime * 8.0f + lp.x) * 0.1f +
                      std::sin(m_gameTime * 13.0f + lp.y) * 0.1f;
      m_renderer->drawQuadAlpha(lp, {120, 120}, 1.0f, 0.9f, 0.5f,
                                0.08f * flicker, 0.0f);
      m_renderer->drawQuadAlpha(lp, {60, 60}, 1.0f, 0.9f, 0.6f,
                                0.15f * flicker, 0.0f);
      m_renderer->drawQuad(lp, {16, 16}, 1.0f * flicker, 0.85f * flicker,
                           0.4f * flicker, 0.0f);
    }

    // Tables (T)
    auto tables = m_floors[mCurrentFloor].findTiles('T');
    for (const auto &tp : tables) {
      m_renderer->drawQuad(tp, {80, 50}, 0.45f, 0.3f, 0.2f, 0.0f);
      m_renderer->drawQuad(tp, {70, 40}, 0.55f, 0.4f, 0.25f, 0.0f);
      m_renderer->drawQuad({tp.x - 15.0f, tp.y + 3.0f}, {12, 8}, 0.3f, 0.3f,
                           0.5f, 0.2f);
      m_renderer->drawQuad({tp.x + 15.0f, tp.y - 2.0f}, {8, 10}, 0.6f, 0.2f,
                           0.2f, -0.1f);
    }

    // Shelves (F) — tall furniture with horizontal lines
    auto shelves = m_floors[mCurrentFloor].findTiles('F');
    for (const auto &fp : shelves) {
      m_renderer->drawQuad(fp, {70, 90}, 0.40f, 0.28f, 0.15f, 0.0f);
      m_renderer->drawQuad(fp, {64, 84}, 0.50f, 0.35f, 0.20f, 0.0f);
      // Shelf planks
      for (int s = -1; s <= 1; ++s) {
        m_renderer->drawQuad({fp.x, fp.y + s * 25.0f}, {60, 4}, 0.38f, 0.25f,
                             0.12f, 0.0f);
      }
      // Items on shelves
      m_renderer->drawQuad({fp.x - 12.0f, fp.y + 12.0f}, {10, 14}, 0.3f, 0.5f,
                           0.7f, 0.1f);
      m_renderer->drawQuad({fp.x + 14.0f, fp.y - 12.0f}, {8, 10}, 0.7f, 0.3f,
                           0.3f, -0.1f);
    }

    // Barrels (V) — round-ish containers
    auto barrels = m_floors[mCurrentFloor].findTiles('V');
    for (const auto &vp : barrels) {
      // Barrel body (simulated with overlapping quads)
      m_renderer->drawQuad(vp, {44, 56}, 0.50f, 0.35f, 0.15f, 0.0f);
      m_renderer->drawQuad(vp, {48, 48}, 0.55f, 0.38f, 0.18f, 0.0f);
      m_renderer->drawQuad(vp, {44, 40}, 0.60f, 0.42f, 0.20f, 0.0f);
      // Metal bands
      m_renderer->drawQuad({vp.x, vp.y + 18.0f}, {46, 4}, 0.35f, 0.35f,
                           0.40f, 0.0f);
      m_renderer->drawQuad({vp.x, vp.y - 18.0f}, {46, 4}, 0.35f, 0.35f,
                           0.40f, 0.0f);
    }

    // Columns / Pillars (C) — structural
    auto columns = m_floors[mCurrentFloor].findTiles('C');
    for (const auto &cp : columns) {
      m_renderer->drawQuad(cp, {36, 90}, 0.55f, 0.52f, 0.50f, 0.0f);
      m_renderer->drawQuad(cp, {30, 80}, 0.62f, 0.58f, 0.55f, 0.0f);
      // Base and capital
      m_renderer->drawQuad({cp.x, cp.y + 40.0f}, {42, 10}, 0.50f, 0.48f,
                           0.45f, 0.0f);
      m_renderer->drawQuad({cp.x, cp.y - 40.0f}, {42, 10}, 0.50f, 0.48f,
                           0.45f, 0.0f);
    }

    // Bookshelves (K) — wide with book-colored bands
    auto bookshelves = m_floors[mCurrentFloor].findTiles('K');
    for (const auto &kp : bookshelves) {
      m_renderer->drawQuad(kp, {90, 80}, 0.35f, 0.22f, 0.12f, 0.0f);
      m_renderer->drawQuad(kp, {82, 72}, 0.42f, 0.28f, 0.15f, 0.0f);
      // Book rows in different colors
      float bookColors[][3] = {
          {0.6f, 0.2f, 0.2f}, {0.2f, 0.4f, 0.6f}, {0.2f, 0.5f, 0.3f},
          {0.6f, 0.5f, 0.2f}, {0.5f, 0.2f, 0.5f}};
      for (int b = 0; b < 5; ++b) {
        float bx = kp.x - 30.0f + b * 15.0f;
        m_renderer->drawQuad({bx, kp.y + 15.0f}, {12, 20},
                             bookColors[b][0], bookColors[b][1],
                             bookColors[b][2], 0.0f);
        m_renderer->drawQuad({bx + 3.0f, kp.y - 15.0f}, {10, 18},
                             bookColors[(b + 2) % 5][0],
                             bookColors[(b + 2) % 5][1],
                             bookColors[(b + 2) % 5][2], 0.05f);
      }
    }

    // Lockers / Cabinets (N) — metal storage
    auto lockers = m_floors[mCurrentFloor].findTiles('N');
    for (const auto &np : lockers) {
      m_renderer->drawQuad(np, {50, 80}, 0.45f, 0.45f, 0.50f, 0.0f);
      m_renderer->drawQuad(np, {44, 74}, 0.50f, 0.50f, 0.55f, 0.0f);
      // Door split line
      m_renderer->drawQuad(np, {2, 70}, 0.35f, 0.35f, 0.40f, 0.0f);
      // Handles
      m_renderer->drawQuad({np.x - 8.0f, np.y + 5.0f}, {4, 10}, 0.3f, 0.3f,
                           0.35f, 0.0f);
      m_renderer->drawQuad({np.x + 8.0f, np.y + 5.0f}, {4, 10}, 0.3f, 0.3f,
                           0.35f, 0.0f);
    }
  }

  // ── Render aliens ──
  renderAliens();

  // ── Render particles ──
  renderParticles();

  // ── Player ──
  {
    Vector2 drawPos = {m_playerPos.x, m_playerPos.y + m_lungeHeight};
    float playerSize = 64.0f;

    // Shadow when jumping
    if (m_lungeHeight > 2.0f) {
      float shadowAlpha = 0.3f * (m_lungeHeight / 40.0f);
      float shadowScale = 1.0f - (m_lungeHeight / 80.0f);
      m_renderer->drawQuadAlpha(m_playerPos,
                                {playerSize * shadowScale, playerSize * shadowScale * 0.5f},
                                0.0f, 0.0f, 0.0f, shadowAlpha, 0.0f);
    }

    if (m_playerTexture) {
      m_renderer->drawQuad(drawPos, {playerSize, playerSize}, m_playerTexture.get(),
                           m_playerRotation);
    } else {
      m_renderer->drawQuad(drawPos, {playerSize, playerSize}, 0.9f, 0.2f, 0.8f,
                           m_playerRotation);
    }
  }

  // Carried rabbit on player
  if (m_hasRabbit) {
    float bob = std::sin(m_gameTime * 5.0f) * 3.0f;
    m_renderer->drawQuad({m_playerPos.x + 20.0f, m_playerPos.y + 20.0f + bob},
                         {20, 20}, 1.0f, 1.0f, 1.0f, 0.0f);
  }

  // Editor overlay
  renderEditorOverlay();

  // ── UI: elevator panel ──
  if (mNearElevator) {
    float uiWX = m_playerPos.x;
    float uiWY = m_playerPos.y + 120.0f;

    m_renderer->drawQuadAlpha({uiWX, uiWY}, {200, 120}, 0.1f, 0.1f, 0.15f,
                              0.9f, 0.0f);
    m_renderer->drawQuadAlpha({uiWX, uiWY}, {196, 116}, 0.2f, 0.2f, 0.25f,
                              0.8f, 0.0f);

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

  // ── Stair prompt ──
  if (mCurrentFloor >= 0 &&
      mCurrentFloor < static_cast<int>(m_floors.size())) {
    auto stairPos = m_floors[mCurrentFloor].findTiles('S');
    for (const auto &sp : stairPos) {
      if (Vector2::distanceSq(m_playerPos, sp) < 120.0f * 120.0f) {
        float promptY = sp.y + 60.0f;
        m_renderer->drawQuadAlpha({sp.x, promptY}, {100, 22}, 0.0f, 0.0f,
                                  0.0f, 0.6f, 0.0f);
        m_renderer->drawQuad({sp.x - 20.0f, promptY}, {14, 14}, 0.8f, 0.8f,
                             0.2f, 0.0f);
        m_renderer->drawQuad({sp.x + 20.0f, promptY}, {14, 14}, 0.8f, 0.4f,
                             0.2f, 0.0f);
        break;
      }
    }
  }

  // ── HUD ──
  renderHUD();

  // ── Minimap ──
  renderMinimap();

  // ── Waypoint arrow ──
  if (m_gameState == GameState::Playing) {
    renderWaypoint();
  }

  // ── Hint messages ──
  renderHints();

  // ── Game over / win ──
  if (m_gameState != GameState::Playing) {
    renderGameOver();
  }

  // Restore camera
  m_renderer->setCameraPosition(m_cameraPos, shakeOffset);

  m_renderer->endFrame();
}

// ─────────────────────── run ───────────────────────────────

void Application::run() {
  std::cout << "\n🎮 XENOCIDE running!\n";
  std::cout << "   WASD/Arrows — move | Mouse — aim | Shift — sprint\n";
  std::cout << "   SPACE — attack aliens | 1/2/3 — elevator floors\n";
  std::cout << "   E/Q — stairs up/down | F — pick up rabbit\n";
  std::cout << "   ESC — exit | R — restart\n";
  std::cout << "   F1 — toggle map editor\n";
  std::cout << "   [Editor] LMB — place | RMB — erase | [] — cycle tiles | "
               "F5 — save\n\n";
  std::cout << "   🎯 OBJECTIVE: Find the rabbit on floor 3,\n";
  std::cout << "      bring it to the EXIT DOOR on floor 1!\n";
  std::cout << "      ⚠️  Beware of aliens — they get angry when you grab "
               "the rabbit!\n";
  std::cout << "      💀 Press SPACE to stomp on aliens!\n";
  std::cout << "      🧭 Follow the waypoint arrow to your objective!\n\n";

  while (!glfwWindowShouldClose(m_window) && m_running) {
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;

    // Cap deltaTime to avoid physics explosions
    deltaTime = std::min(deltaTime, 0.05f);

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
