#pragma once

namespace mine {

class Texture {
public:
    Texture();
    ~Texture();

    bool loadFromFile(const char* path);
    void bind(unsigned int unit = 0) const;
    void unbind() const;

    unsigned int getId() const { return m_id; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    unsigned int m_id = 0;
    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;
};

} // namespace mine
