#include "mine/graphics/Texture.hpp"
/*
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <iostream>

namespace mine {

Texture::Texture() {
    glGenTextures(1, &m_id);
}

Texture::~Texture() {
    if (m_id) {
        glDeleteTextures(1, &m_id);
    }
}

bool Texture::loadFromFile(const char* path) {
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &m_width, &m_height, &m_channels, 0);
    
    if (!data) {
        std::cerr << "❌ Failed to load texture: " << path << "\n";
        std::cerr << "   stb_image error: " << stbi_failure_reason() << "\n";
        return false;
    }

    GLenum format = GL_RGB;
    if (m_channels == 4) format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Настройки текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "✅ Texture loaded: " << path << " (" << m_width << "x" << m_height << ")\n";
    return true;
}

void Texture::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace mine
*/
