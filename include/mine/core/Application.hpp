#pragma once

#include "mine/math/Vector2.hpp"
#include "mine/scene/TileMap.hpp"
#include "mine/utils/TextureManager.hpp"
#include <memory>
#include <random>
#include <string>
#include <vector>

// Forward-declare miniaudio types so we don't pull the giant header here
struct ma_engine;
struct ma_sound;

struct GLFWwindow;

namespace mine {
class Texture;
class Renderer;

// ─── Alien monster ─────────────────────────────────────────
struct Alien {
  Vector2 pos;
  float rotation = 0.0f;
  int floor = 0;
  float speed = 80.0f;

  // AI states
  enum class State { Idle, Patrol, Chase, Attack, Dying };
  State state = State::Idle;

  // Patrol
  Vector2 patrolTarget;
  float idleTimer = 0.0f;
  float idleDuration = 2.0f;

  // Vision
  float sightRange = 400.0f;
  float sightAngle = 1.2f; // radians (about 70 deg half-cone)

  // Attack
  float attackCooldown = 0.0f;
  float attackRange = 60.0f;
  float damage = 8.0f;

  // Health
  float hp = 40.0f;
  float maxHp = 40.0f;

  // Death animation
  float deathTimer = 0.0f;
  float deathDuration = 0.8f;
  float squash = 1.0f;  // 1.0 = normal, shrinks to 0

  // Visual
  float animTimer = 0.0f;
  bool alive = true;
};

// ─── Particle (for fun VFX) ──────────────────────────────
struct Particle {
  Vector2 pos, vel;
  float life = 0.0f, maxLife = 1.0f;
  float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
  float size = 6.0f;
  float gravity = 0.0f;     // downward pull for blood drops
  float rotation = 0.0f;
  float rotSpeed = 0.0f;
};

// ─── Game state enum ─────────────────────────────────────
enum class GameState { Playing, Won, Dead };

class Application {
public:
  Application(int width, int height, const char *title);
  ~Application();

  void run();
  void shakeCamera(float intensity, float duration);

private:
  // --- Map system ---
  TextureManager m_texManager;
  std::vector<TileMap> m_floors;
  std::string m_assetsDir;

  // --- Floor / navigation ---
  int mCurrentFloor = 0;
  bool mElevatorCooldown = false;
  bool mNearElevator = false;
  int mSelectedFloor = 0;
  int mWindowWidth = 0;
  int mWindowHeight = 0;

  // --- Editor ---
  bool m_editorMode = false;
  int m_editorTileIndex = 1;
  bool m_f1Pressed = false;
  bool m_f5Pressed = false;
  bool m_bracketPressed = false;

  // --- Methods ---
  bool checkWallCollision(const Vector2 &newPos, float radius = 32.0f);
  bool checkWallCollisionOnFloor(int floor, const Vector2 &pos, float radius);
  void processInput(float deltaTime);
  void update(float deltaTime);
  void render();
  void loadMaps();
  void renderEditorOverlay();
  void renderHUD();
  void renderGameOver();

  // --- Core ---
  GLFWwindow *m_window = nullptr;
  Renderer *m_renderer = nullptr;
  bool m_running = true;
  float m_lastFrameTime = 0.0f;
  std::shared_ptr<Texture> m_playerTexture;
  Vector2 m_playerPos;
  float m_playerRotation = 0.0f;

  // Camera
  Vector2 m_cameraPos = {0.0f, 0.0f};
  float m_cameraSmooth = 0.1f;

  // Camera shake
  float m_cameraShakeIntensity = 0.0f;
  float m_cameraShakeDuration = 0.0f;
  float m_cameraShakeTimer = 0.0f;

  // Random engine
  std::mt19937 m_randomEngine;
  std::uniform_real_distribution<float> m_randomDist;

  // ═══════════ PLAYER ATTACK ═══════════
  float m_playerAttackCooldown = 0.0f;
  float m_playerAttackRange = 100.0f;
  float m_playerAttackDamage = 25.0f;
  bool m_spacePressed = false;
  // Jump/lunge animation
  bool m_isLunging = false;
  float m_lungeTimer = 0.0f;
  float m_lungeDuration = 0.25f;
  Vector2 m_lungeStart;
  Vector2 m_lungeTarget;
  float m_lungeHeight = 0.0f;  // visual Y offset for "jump"
  int m_killCount = 0;

  // ═══════════ MUSIC ═══════════
  ma_engine* m_audioEngine = nullptr;
  ma_sound* m_musicSound = nullptr;
  bool m_musicPlaying = false;
  void initAudio();
  void shutdownAudio();

  // ═══════════ NEW SYSTEMS ═══════════

  // --- Player stats ---
  float m_playerHP = 100.0f;
  float m_playerMaxHP = 100.0f;
  float m_playerStamina = 100.0f;
  float m_playerMaxStamina = 100.0f;
  float m_staminaRegenRate = 20.0f;   // per second
  float m_staminaDrainRate = 35.0f;   // per second while sprinting
  float m_staminaRegenDelay = 0.5f;   // seconds after sprint before regen
  float m_staminaRegenTimer = 0.0f;
  bool m_isSprinting = false;
  float m_damageFlashTimer = 0.0f;

  // --- Rabbit rescue ---
  bool m_hasRabbit = false;
  bool m_rabbitPickedUp = false;
  Vector2 m_rabbitPos;
  int m_rabbitFloor = 2;  // 3rd floor (index 2)
  float m_rabbitBob = 0.0f; // bobbing animation

  // --- Exit ---
  Vector2 m_exitPos;
  int m_exitFloor = 0;  // 1st floor (index 0)

  // --- Aliens ---
  std::vector<Alien> m_aliens;
  void spawnAliens();
  void updateAliens(float deltaTime);
  void renderAliens();
  bool hasLineOfSight(int floor, const Vector2 &from, const Vector2 &to);
  void playerAttack();
  void spawnBlood(const Vector2 &pos, int count);

  // --- Particles ---
  std::vector<Particle> m_particles;
  void spawnParticles(const Vector2 &pos, int count, float r, float g, float b);
  void spawnParticlesEx(const Vector2 &pos, int count, float r, float g, float b,
                        float sizeMin, float sizeMax, float speedMin, float speedMax,
                        float lifeMin, float lifeMax, float gravity = 0.0f);
  void updateParticles(float deltaTime);
  void renderParticles();

  // --- Game state ---
  GameState m_gameState = GameState::Playing;
  float m_gameTime = 0.0f;
  float m_winTimer = 0.0f;

  // --- Fun extras ---
  float m_footstepTimer = 0.0f;
  int m_stepCount = 0;
  float m_heartbeatTimer = 0.0f; // when HP is low, pulsing effect

  // --- Hint / message system ---
  struct HintMessage {
    float timer = 0.0f;
    float duration = 3.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f;
    int iconType = 0; // 0=info, 1=warning, 2=objective, 3=funny
  };
  std::vector<HintMessage> m_hints;
  float m_hintCooldown = 0.0f;
  bool m_shownStartHint = false;
  bool m_shownRabbitHint = false;
  bool m_shownDoorHint = false;
  bool m_shownAttackHint = false;
  int m_lastHintFloor = -1;
  void showHint(float duration, float r, float g, float b, int icon);
  void updateHints(float deltaTime);
  void renderHints();
  void renderWaypoint();

  // --- Minimap ---
  void renderMinimap();
};

} // namespace mine
