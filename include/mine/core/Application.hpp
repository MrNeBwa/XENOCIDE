#pragma once

#include "mine/math/Vector2.hpp"
#include <memory>
#include <random> // ADD THIS FOR RANDOM ENGINE
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
  struct GameObject {
    Vector2 position;
    Vector2 size;
    int floorLevel;
    std::shared_ptr<Texture> texture; // nullptr = цветной объект
    float r, g, b;                    // Цвет (если нет текстуры)
    bool isSolid = false;             // Для стен
  };

  struct Elevator {
    Vector2 position;
    int floorLevel;
    bool isLeftSide; // true = левые углы, false = правые
  };

  struct Stair {
    Vector2 position;
    int floorLevel;
    bool isLeftSide; // true = левые углы, false = правые
  };

  // --- Данные уровня ---
  std::vector<GameObject> m_gameObjects;
  std::vector<Elevator> m_elevators;
  std::vector<Stair> m_stairs;

  int mCurrentFloor = 0;
  bool mElevatorCooldown = false;
  bool mNearElevator = false;
  int mSelectedFloor = 0; // Для UI лифта

  int mWindowWidth = 0;
  int mWindowHeight = 0;

  // --- Методы ---
  bool canUseElevator(const Vector2 &playerPos,
                      Elevator **outElevator = nullptr);
  bool canUseStair(const Vector2 &playerPos, Stair **outStair = nullptr);
  bool checkWallCollision(const Vector2 &newPos, float radius = 32.0f);
  void loadFloorTexture(std::shared_ptr<Texture> &tex, const std::string &path);
  void processInput(float deltaTime); // FIXED: added parameter
  void update(float deltaTime);
  void render();

  void generateFloors();

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

  // Proper random engine
  std::mt19937 m_randomEngine;
  std::uniform_real_distribution<float> m_randomDist;
  // World size used for floor generation and background
  float m_worldSize = 2000.0f;
};

} // namespace mine
