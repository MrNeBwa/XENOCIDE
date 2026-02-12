#pragma once

namespace mine {

class Texture {
public:
    Texture() = default;
    bool loadFromFile(const char* path) { (void)path; return false; }
    void bind(unsigned int unit = 0) const { (void)unit; }
    unsigned int getId() const { return 0; }
    int getWidth() const { return 64; }
    int getHeight() const { return 64; }
};

} // namespace mine
