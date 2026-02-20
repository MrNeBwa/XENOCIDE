#pragma once

#include "mine/math/Vector2.hpp"
#include "mine/graphics/Texture.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mine {

class Renderer;
class TextureManager;

struct TileDef {
    char symbol = ' ';
    bool solid = false;
    float r = 0.0f, g = 0.0f, b = 0.0f;
    std::string texturePath;
    std::shared_ptr<Texture> texture;
    std::string name;
};

class TileMap {
public:
    TileMap() = default;

    bool load(const std::string &path, TextureManager &texMgr,
              const std::string &assetsDir);
    bool save(const std::string &path) const;

    void render(Renderer &renderer) const;

    // Grid operations
    char getTile(int gx, int gy) const;
    void setTile(int gx, int gy, char symbol);

    // Coordinate conversion  (grid y=0 is top of world)
    std::pair<int, int> worldToGrid(const Vector2 &worldPos) const;
    Vector2 gridToWorld(int gx, int gy) const;

    // Collision with solid tiles
    bool checkCollision(const Vector2 &pos, float radius) const;

    // Find first tile of a given symbol → world position
    Vector2 findSpawn() const;
    std::vector<Vector2> findTiles(char symbol) const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getTileSize() const { return m_tileSize; }
    float getWorldWidth() const { return m_width * m_tileSize; }
    float getWorldHeight() const { return m_height * m_tileSize; }

    const std::vector<TileDef> &getTileDefs() const { return m_tileDefs; }
    const TileDef *getDefForSymbol(char c) const;

    float bgR = 0.1f, bgG = 0.1f, bgB = 0.15f;

private:
    int m_width = 0, m_height = 0;
    float m_tileSize = 100.0f;
    std::vector<TileDef> m_tileDefs;
    std::vector<std::vector<char>> m_grid;
    std::unordered_map<char, int> m_defIndex;
};

} // namespace mine
