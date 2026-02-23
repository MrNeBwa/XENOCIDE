#pragma once
#include <string>

namespace mine {

class Texture {
public:
  Texture(const std::string &path);
  ~Texture();

  void bind(unsigned int slot = 0) const;
  void unbind() const;

  int getWidth() const { return m_width; }
  int getHeight() const { return m_height; }

private:
  unsigned int m_id;
  std::string m_filePath;
  int m_width, m_height, m_channels;
};

} 
