#pragma once

namespace mine {

class Shader {
public:
    Shader();
    ~Shader();

    bool loadFromFile(const char* vertexPath, const char* fragmentPath);
    void use() const;

    // Униформы
    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;
    void setMat4(const char* name, const float* matrix) const; // row-major

private:
    unsigned int m_id = 0;
};

} // namespace mine
