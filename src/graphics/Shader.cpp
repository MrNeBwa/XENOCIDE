#include "mine/graphics/Shader.hpp"
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace mine {

Shader::Shader() = default;
Shader::~Shader() {
    if (m_id) glDeleteProgram(m_id);
}

static unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cerr << "❌ Shader compilation error:\n" << message << "\n";
        glDeleteShader(id);
        return 0;
    }

    return id;
}

bool Shader::loadFromFile(const char* vertexPath, const char* fragmentPath) {
    // Чтение файлов
    std::ifstream vFile(vertexPath);
    std::ifstream fFile(fragmentPath);
    if (!vFile.is_open() || !fFile.is_open()) {
        std::cerr << "❌ Failed to open shader files:\n";
        std::cerr << "   Vertex: " << vertexPath << "\n";
        std::cerr << "   Fragment: " << fragmentPath << "\n";
        return false;
    }

    std::stringstream vBuffer, fBuffer;
    vBuffer << vFile.rdbuf();
    fBuffer << fFile.rdbuf();
    vFile.close();
    fFile.close();

    std::string vSourceStr = vBuffer.str();
    std::string fSourceStr = fBuffer.str();
    const char* vSource = vSourceStr.c_str();
    const char* fSource = fSourceStr.c_str();

    // Компиляция
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fSource);
    if (!vs || !fs) return false;

    // Линковка
    m_id = glCreateProgram();
    glAttachShader(m_id, vs);
    glAttachShader(m_id, fs);
    glLinkProgram(m_id);
    glValidateProgram(m_id);

    glDeleteShader(vs);
    glDeleteShader(fs);

    std::cout << "✅ Shader loaded: " << vertexPath << " + " << fragmentPath << "\n";
    return true;
}

void Shader::use() const {
    glUseProgram(m_id);
}

void Shader::setInt(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(m_id, name), value);
}

void Shader::setFloat(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(m_id, name), value);
}

void Shader::setVec4(const char* name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(m_id, name), x, y, z, w);
}

void Shader::setMat4(const char* name, const float* matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(m_id, name), 1, GL_FALSE, matrix);
}

} // namespace mine
