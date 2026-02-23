#include "mine/graphics/Texture.hpp"
#include <glad/glad.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace mine {

Texture::Texture(const std::string &path)
    : m_id(0), m_filePath(path), m_width(0), m_height(0), m_channels(0) {
  stbi_set_flip_vertically_on_load(true);

  unsigned char *data =
      stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 4);

  if (!data) {
    std::cerr << "❌ Failed to load texture: " << path << "\n";
    return;
  }

  glGenTextures(1, &m_id);
  glBindTexture(GL_TEXTURE_2D, m_id);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

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

} 
