#include "mine/graphics/Texture.hpp"
#include "stb/stb_image.h" // Проверь этот путь!
#include <glad/glad.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
namespace mine {

Texture::Texture(const std::string &path)
    : m_id(0), m_filePath(path), m_width(0), m_height(0), m_channels(0) {
  // Переворачиваем картинку, так как в OpenGL Y растет вверх
  stbi_set_flip_vertically_on_load(true);

  unsigned char *data =
      stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 4);

  if (!data) {
    std::cerr << "❌ Failed to load texture: " << path << "\n";
    return;
  }

  glGenTextures(1, &m_id);
  glBindTexture(GL_TEXTURE_2D, m_id);

  // GL_RGBA для прозрачных картинок (png), GL_RGB для jpg
  GLenum format = (m_channels == 4) ? GL_RGBA : GL_RGB;

  // Загрузка данных
  glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format,
               GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  // Настройки фильтрации (GL_NEAREST для пикселей, GL_LINEAR для гладкости)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D, 0);
  stbi_image_free(data);
}

Texture::~Texture() { glDeleteTextures(1, &m_id); }

void Texture::bind(unsigned int slot) const {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

} // namespace mine
