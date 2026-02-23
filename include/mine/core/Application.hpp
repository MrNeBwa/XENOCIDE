#pragma once

#include "mine/math/Vector2.hpp"
#include "mine/scene/TileMap.hpp"
#include "mine/utils/TextureManager.hpp"
#include <memory>
#include <random>
#include <string>
#include <vector>

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

  enum class State { Idle, Patrol, Chase, Attack, Dying };
  State state = State::Idle;

  Vector2 patrolTarget;
  float idleTimer = 0.0f;
  float idleDuration = 2.0f;

  float sightRange = 400.0f;
  float sightAngle = 1.2f; 

  float attackCooldown = 0.0f;
  float attackRange = 60.0f;
  float damage = 8.0f;

  float hp = 40.0f;
  float maxHp = 40.0f;

  float deathTimer = 0.0f;
  float deathDuration = 0.8f;
  float squash = 1.0f;  

  float animTimer = 0.0f;
  bool alive = true;
  bool elite = false;
};

// ─── Particle (for fun VFX) ──────────────────────────────
struct Particle {
  Vector2 pos, vel;
  float life = 0.0f, maxLife = 1.0f;
  float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
  float size = 6.0f;
  float gravity = 0.0f;     
  float rotation = 0.0f;
  float rotSpeed = 0.0f;
};

// ─── Health pickup ───────────────────────────────────────
struct HealthPickup {
  Vector2 pos;
  int floor = 0;
  float healAmount = 25.0f;
  bool active = true;
  float respawnTimer = 0.0f;
  float respawnDelay = 30.0f;
  float bobTimer = 0.0f;
};

// ─── Gun pickup ──────────────────────────────────────────
struct GunPickup {
  Vector2 pos;
  int floor = 0;
  bool active = true;
  float bobTimer = 0.0f;
};

// ─── Projectile (bullet) ─────────────────────────────────
struct Projectile {
  Vector2 pos;
  Vector2 vel;
  int floor = 0;
  float damage = 40.0f;
  float life = 2.0f;
  float rotation = 0.0f;
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
  TextureManager m_texManager;
  std::vector<TileMap> m_floors;
  std::string m_assetsDir;

  int mCurrentFloor = 0;
  bool mElevatorCooldown = false;
  bool mStairCooldown = false;
  bool mNearElevator = false;
  int mSelectedFloor = 0;
  int mWindowWidth = 0;
  int mWindowHeight = 0;

  bool m_editorMode = false;
  int m_editorTileIndex = 1;
  bool m_f1Pressed = false;
  bool m_f5Pressed = false;
  bool m_bracketPressed = false;

  bool checkWallCollision(const Vector2 &newPos, float radius = 32.0f);
  bool checkWallCollisionOnFloor(int floor, const Vector2 &pos, float radius);
  void processInput(float deltaTime);
  void update(float deltaTime);
  void render();
  void loadMaps();
  void renderEditorOverlay();
  void renderHUD();
  void renderGameOver();

  GLFWwindow *m_window = nullptr;
  Renderer *m_renderer = nullptr;
  bool m_running = true;
  float m_lastFrameTime = 0.0f;
  std::shared_ptr<Texture> m_playerTexture;
  Vector2 m_playerPos;
  float m_playerRotation = 0.0f;

  Vector2 m_cameraPos = {0.0f, 0.0f};
  float m_cameraSmooth = 0.1f;

  float m_cameraShakeIntensity = 0.0f;
  float m_cameraShakeDuration = 0.0f;
  float m_cameraShakeTimer = 0.0f;

  std::mt19937 m_randomEngine;
  std::uniform_real_distribution<float> m_randomDist;

  float m_playerAttackCooldown = 0.0f;
  float m_playerAttackRange = 100.0f;
  float m_playerAttackDamage = 25.0f;
  bool m_spacePressed = false;
  bool m_isLunging = false;
  float m_lungeTimer = 0.0f;
  float m_lungeDuration = 0.25f;
  Vector2 m_lungeStart;
  Vector2 m_lungeTarget;
  float m_lungeHeight = 0.0f;  
  int m_killCount = 0;

  
  bool m_isDashing = false;
  float m_dashTimer = 0.0f;
  float m_dashDuration = 0.15f;
  float m_dashSpeed = 1200.0f;
  float m_dashCooldown = 0.0f;
  float m_dashCooldownMax = 0.8f;
  float m_dashStaminaCost = 25.0f;
  Vector2 m_dashDir;
  bool m_rmbPressed = false;

  ma_engine* m_audioEngine = nullptr;
  ma_sound* m_musicSound = nullptr;
  bool m_musicPlaying = false;
  void initAudio();
  void shutdownAudio();


  float m_playerHP = 100.0f;
  float m_playerMaxHP = 100.0f;
  float m_playerStamina = 100.0f;
  float m_playerMaxStamina = 100.0f;
  float m_staminaRegenRate = 20.0f;   
  float m_staminaDrainRate = 35.0f;   
  float m_staminaRegenDelay = 0.5f;   
  float m_staminaRegenTimer = 0.0f;
  bool m_isSprinting = false;
  float m_damageFlashTimer = 0.0f;

  bool m_hasRabbit = false;
  bool m_rabbitPickedUp = false;
  Vector2 m_rabbitPos;
  int m_rabbitFloor = 2;  
  float m_rabbitBob = 0.0f; 

  
  bool m_hasKeycard = false;
  Vector2 m_keycardPos;
  int m_keycardFloor = 1;
  float m_keycardBob = 0.0f;
  bool m_shownKeycardHint = false;

  Vector2 m_exitPos;
  int m_exitFloor = 0;  

  std::vector<Alien> m_aliens;
  void spawnAliens();
  void spawnAlienReinforcements();
  void updateAliens(float deltaTime);
  void renderAliens();
  bool hasLineOfSight(int floor, const Vector2 &from, const Vector2 &to);
  void playerAttack();
  void spawnBlood(const Vector2 &pos, int count);

  std::vector<Particle> m_particles;
  void spawnParticles(const Vector2 &pos, int count, float r, float g, float b);
  void spawnParticlesEx(const Vector2 &pos, int count, float r, float g, float b,
                        float sizeMin, float sizeMax, float speedMin, float speedMax,
                        float lifeMin, float lifeMax, float gravity = 0.0f);
  void updateParticles(float deltaTime);
  void renderParticles();

  GameState m_gameState = GameState::Playing;
  float m_gameTime = 0.0f;
  float m_winTimer = 0.0f;

  float m_footstepTimer = 0.0f;
  int m_stepCount = 0;
  float m_heartbeatTimer = 0.0f; 

  struct HintMessage {
    std::string text;
    float timer = 0.0f;
    float duration = 3.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f;
    int iconType = 0; 
  };
  std::vector<HintMessage> m_hints;
  float m_hintCooldown = 0.0f;
  bool m_shownStartHint = false;
  bool m_shownRabbitHint = false;
  bool m_shownDoorHint = false;
  bool m_shownAttackHint = false;
  int m_lastHintFloor = -1;
  void showHint(const std::string &text, float duration, float r, float g, float b, int icon);
  void updateHints(float deltaTime);
  void renderHints();
  void renderWaypoint();

  std::vector<HealthPickup> m_healthPickups;
  void spawnHealthPickups();
  void updateHealthPickups(float deltaTime);
  void renderHealthPickups();

  
  std::vector<GunPickup> m_gunPickups;
  bool m_hasGun = false;
  int m_gunAmmo = 0;
  bool m_lmbPressed = false;
  void spawnGunPickups();
  void updateGunPickups(float deltaTime);
  void renderGunPickups();

  
  std::vector<Projectile> m_projectiles;
  void shootGun();
  void updateProjectiles(float deltaTime);
  void renderProjectiles();

  float m_combatTimer = 0.0f;
  float m_outOfCombatDelay = 5.0f;
  float m_passiveRegenRate = 2.0f; 

  void renderMinimap();
};

} 
