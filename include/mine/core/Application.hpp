#pragma once

#include "mine/math/Vector2.hpp"
#include "mine/scene/TileMap.hpp"
#include "mine/utils/TextureManager.hpp"
#include <memory>
#include <random>
#include <string>
#include <vector>

struct GLFWwindow;

namespace mine {
class Texture;
class Renderer;

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
  void processInput(float deltaTime);
  void update(float deltaTime);
  void render();
  void loadMaps();
  void renderEditorOverlay();

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

  // Spacebar state tracker
  bool m_hasShaken = false;

  // Random engine
  std::mt19937 m_randomEngine;
  std::uniform_real_distribution<float> m_randomDist;
};

} // namespace mine
