#pragma once

namespace mine {

class Shader {
public:
    Shader() = default;
    bool loadFromFile(const char*, const char*) { return true; }
    void use() const {}
    void setInt(const char*, int) const {}
    void setFloat(const char*, float) const {}
    void setVec4(const char*, float, float, float, float) const {}
    void setMat4(const char*, const float*) const {}
};

} 
