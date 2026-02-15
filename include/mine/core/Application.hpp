#pragma once

#include "mine/math/Vector2.hpp"
#include <memory>
#include <random> // ADD THIS FOR RANDOM ENGINE
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
  void processInput(float deltaTime); // FIXED: added parameter
  void update(float deltaTime);
  void render();

  GLFWwindow *m_window = nullptr;
  Renderer *m_renderer = nullptr;
  bool m_running = true;
  float m_lastFrameTime = 0.0f;
  std::shared_ptr<Texture> m_playerTexture;
  Vector2 m_playerPos; // Removed inconsistent default init
  float m_playerRotation = 0.0f;

  // Camera
  Vector2 m_cameraPos = {0.0f, 0.0f};
  Vector2 m_cameraTarget = {0.0f, 0.0f};
  float m_cameraSmooth = 0.1f;

  // Camera shake
  float m_cameraShakeIntensity = 0.0f;
  float m_cameraShakeDuration = 0.0f;
  float m_cameraShakeTimer = 0.0f;

  // Spacebar state tracker (FIXES static variable bug)
  bool m_hasShaken = false;

  // Proper random engine (FIXES unseeded rand)
  std::mt19937 m_randomEngine;
  std::uniform_real_distribution<float> m_randomDist;
};

} // namespace mine
