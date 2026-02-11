#pragma once

#include "mine/math/Vector2.hpp"

namespace mine {

class Renderer {
public:
    Renderer(int width, int height);
    void beginFrame();
    void endFrame();
    void drawColoredQuad(const Vector2& pos, const Vector2& size, float r, float g, float b);
};

} // namespace mine
