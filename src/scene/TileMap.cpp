#include "mine/scene/TileMap.hpp"
#include "mine/graphics/Renderer.hpp"
#include "mine/utils/TextureManager.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace mine {

// ─────────────────────── load ───────────────────────
bool TileMap::load(const std::string &path, TextureManager &texMgr,
                   const std::string &assetsDir) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "❌ Failed to open map: " << path << "\n";
        return false;
    }

    m_tileDefs.clear();
    m_grid.clear();
    m_defIndex.clear();

    TileDef emptyDef;
    emptyDef.symbol = ' ';
    emptyDef.name = "empty";
    m_tileDefs.push_back(emptyDef);
    m_defIndex[' '] = 0;

    std::string line;
    bool readingGrid = false;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        if (line[0] == '#')
            continue;

        if (!readingGrid) {
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;

            if (keyword == "tilesize") {
                iss >> m_tileSize;
            } else if (keyword == "bgcolor") {
                iss >> bgR >> bgG >> bgB;
            } else if (keyword == "tile") {
                TileDef def;
                int solid;
                iss >> def.symbol >> solid >> def.r >> def.g >> def.b;
                def.solid = (solid != 0);

                std::string texFile;
                if (iss >> texFile) {
                    def.texturePath = texFile;
                    std::string fullPath = assetsDir + "/textures/" + texFile;
                    def.texture = texMgr.load(fullPath);
                }

                def.name = std::string(1, def.symbol);
                m_defIndex[def.symbol] =
                    static_cast<int>(m_tileDefs.size());
                m_tileDefs.push_back(def);
            } else if (keyword == "grid") {
                readingGrid = true;
            } else {
                bool looksLikeGrid = (line.size() > 3);
                for (char c : line) {
                    if (m_defIndex.find(c) == m_defIndex.end()) {
                        looksLikeGrid = false;
                        break;
                    }
                }
                if (looksLikeGrid) {
                    readingGrid = true;
                    m_grid.push_back(
                        std::vector<char>(line.begin(), line.end()));
                }
            }
        } else {
            if (line[0] == '#')
                continue;
            m_grid.push_back(
                std::vector<char>(line.begin(), line.end()));
        }
    }
    file.close();

    m_height = static_cast<int>(m_grid.size());
    m_width = 0;
    for (const auto &row : m_grid)
        m_width = std::max(m_width, static_cast<int>(row.size()));
    for (auto &row : m_grid)
        row.resize(m_width, ' ');

    std::cout << "✅ Loaded map: " << path << " (" << m_width << "x"
              << m_height << ", tile=" << m_tileSize << ", "
              << m_tileDefs.size() << " defs)\n";
    return true;
}

// ─────────────────────── save ───────────────────────
bool TileMap::save(const std::string &path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "❌ Failed to save map: " << path << "\n";
        return false;
    }

    file << "# XENOCIDE Map (auto-saved)\n";
    file << "tilesize " << m_tileSize << "\n";
    file << "bgcolor " << bgR << " " << bgG << " " << bgB << "\n\n";

    for (size_t i = 1; i < m_tileDefs.size(); ++i) {
        const auto &d = m_tileDefs[i];
        file << "tile " << d.symbol << " " << (d.solid ? 1 : 0) << " "
             << d.r << " " << d.g << " " << d.b;
        if (!d.texturePath.empty())
            file << " " << d.texturePath;
        file << "\n";
    }

    file << "\ngrid\n";
    for (const auto &row : m_grid) {
        for (char c : row)
            file << c;
        file << "\n";
    }

    file.close();
    std::cout << "✅ Saved map: " << path << "\n";
    return true;
}

// ─────────────────────── render ─────────────────────
void TileMap::render(Renderer &renderer) const {
    float worldW = getWorldWidth();
    float worldH = getWorldHeight();

    renderer.drawQuad({worldW / 2.0f, worldH / 2.0f}, {worldW, worldH},
                      bgR, bgG, bgB, 0.0f);

    for (int gy = 0; gy < m_height; ++gy) {
        for (int gx = 0; gx < m_width; ++gx) {
            char c = m_grid[gy][gx];
            if (c == ' ')
                continue;

            const TileDef *def = getDefForSymbol(c);
            if (!def)
                continue;

            Vector2 pos = gridToWorld(gx, gy);
            Vector2 size = {m_tileSize, m_tileSize};

            if (def->texture)
                renderer.drawQuad(pos, size, def->texture.get(), 0.0f);
            else
                renderer.drawQuad(pos, size, def->r, def->g, def->b, 0.0f);
        }
    }
}

// ─────────────────── grid helpers ───────────────────
char TileMap::getTile(int gx, int gy) const {
    if (gx < 0 || gx >= m_width || gy < 0 || gy >= m_height)
        return ' ';
    return m_grid[gy][gx];
}

void TileMap::setTile(int gx, int gy, char symbol) {
    if (gx < 0 || gx >= m_width || gy < 0 || gy >= m_height)
        return;
    m_grid[gy][gx] = symbol;
}

std::pair<int, int> TileMap::worldToGrid(const Vector2 &worldPos) const {
    int gx = static_cast<int>(worldPos.x / m_tileSize);
    float wh = getWorldHeight();
    int gy = static_cast<int>((wh - worldPos.y) / m_tileSize);
    return {gx, gy};
}

Vector2 TileMap::gridToWorld(int gx, int gy) const {
    float x = gx * m_tileSize + m_tileSize / 2.0f;
    float y = getWorldHeight() - (gy * m_tileSize + m_tileSize / 2.0f);
    return {x, y};
}

// ─────────────────── collision ──────────────────────
bool TileMap::checkCollision(const Vector2 &pos, float radius) const {
    float wh = getWorldHeight();
    int gxMin = std::max(0, static_cast<int>((pos.x - radius) / m_tileSize));
    int gxMax =
        std::min(m_width - 1, static_cast<int>((pos.x + radius) / m_tileSize));
    int gyMin =
        std::max(0, static_cast<int>((wh - (pos.y + radius)) / m_tileSize));
    int gyMax = std::min(m_height - 1,
                         static_cast<int>((wh - (pos.y - radius)) / m_tileSize));

    float half = m_tileSize / 2.0f;
    for (int gy = gyMin; gy <= gyMax; ++gy) {
        for (int gx = gxMin; gx <= gxMax; ++gx) {
            char c = m_grid[gy][gx];
            const TileDef *def = getDefForSymbol(c);
            if (!def || !def->solid)
                continue;

            Vector2 tp = gridToWorld(gx, gy);
            float cx = std::clamp(pos.x, tp.x - half, tp.x + half);
            float cy = std::clamp(pos.y, tp.y - half, tp.y + half);
            float dx = pos.x - cx;
            float dy = pos.y - cy;
            if (dx * dx + dy * dy < radius * radius)
                return true;
        }
    }
    return false;
}

// ─────────────────── queries ────────────────────────
Vector2 TileMap::findSpawn() const {
    for (int gy = 0; gy < m_height; ++gy)
        for (int gx = 0; gx < m_width; ++gx)
            if (m_grid[gy][gx] == 'P')
                return gridToWorld(gx, gy);

    return {getWorldWidth() / 2.0f, getWorldHeight() / 2.0f};
}

std::vector<Vector2> TileMap::findTiles(char symbol) const {
    std::vector<Vector2> result;
    for (int gy = 0; gy < m_height; ++gy)
        for (int gx = 0; gx < m_width; ++gx)
            if (m_grid[gy][gx] == symbol)
                result.push_back(gridToWorld(gx, gy));
    return result;
}

const TileDef *TileMap::getDefForSymbol(char c) const {
    auto it = m_defIndex.find(c);
    if (it == m_defIndex.end())
        return nullptr;
    return &m_tileDefs[it->second];
}

} 
