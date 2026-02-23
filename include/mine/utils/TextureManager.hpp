#pragma once

#include "mine/graphics/Texture.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace mine {

class TextureManager {
public:
    std::shared_ptr<Texture> load(const std::string &path) {
        auto it = m_cache.find(path);
        if (it != m_cache.end()) {
            if (auto tex = it->second.lock())
                return tex;
        }
        auto tex = std::make_shared<Texture>(path);
        m_cache[path] = tex;
        return tex;
    }

    void clear() { m_cache.clear(); }

private:
    std::unordered_map<std::string, std::weak_ptr<Texture>> m_cache;
};

} 
