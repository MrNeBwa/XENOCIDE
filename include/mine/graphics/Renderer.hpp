#pragma once

#include "mine/math/Vector2.hpp"

namespace mine {

class Renderer {
public:
  Renderer(int width, int height);
  ~Renderer();

  void beginFrame();
  void endFrame();
  void drawQuad(const Vector2 &pos, const Vector2 &size, float r, float g,
                float b, float rotation = 0.0f);
  void drawQuadAlpha(const Vector2 &pos, const Vector2 &size, float r, float g,
                     float b, float a, float rotation = 0.0f);
  void setCameraPosition(const Vector2 &pos,
                         const Vector2 &shake); // Новая функция
  void drawQuad(const Vector2 &pos, const Vector2 &size,
                const class Texture *texture, float rotation = 0.0f);

  // Switch projection to screen-space (pixel coords, origin bottom-left)
  void beginScreenSpace();

private:
  unsigned int m_shaderProgram = 0;
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_ebo = 0;
  int m_width, m_height;
};

} // namespace mine
