// ⚠️ GLAD ДОЛЖЕН БЫТЬ ПЕРВЫМ — НИЧЕГО ДО ЭТОГО!
#include <glad/glad.h>
// ⚠️ GLFW — ВТОРЫМ, чтобы он не подключил системные GL-заголовки
#include <GLFW/glfw3.h>

// Теперь безопасно подключать остальное
#include "mine/graphics/Renderer.hpp"
#include "mine/graphics/Texture.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace mine {

static unsigned int compileShader(unsigned int type, const char *source) {
  unsigned int id = glCreateShader(type);
  glShaderSource(id, 1, &source, nullptr);
  glCompileShader(id);

  int success;
  glGetShaderiv(id, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(id, 512, nullptr, infoLog);
    std::cerr << "❌ Shader compilation failed:\n" << infoLog << std::endl;
    glDeleteShader(id);
    return 0;
  }
  return id;
}

static unsigned int createShaderProgram() {
  const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        uniform mat4 projection;
        uniform mat4 model;
        void main() {
            gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

  const char *fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform vec3 color;
        uniform bool useTexture;
        uniform sampler2D texture0;
        void main() {
            if (useTexture) {
                FragColor = texture(texture0, TexCoord);
            } else {
                FragColor = vec4(color, 1.0);
            }
        }
    )";

  unsigned int vertexShader =
      compileShader(GL_VERTEX_SHADER, vertexShaderSource);
  unsigned int fragmentShader =
      compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

  unsigned int shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  int success;
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
    std::cerr << "❌ Shader program linking failed:\n" << infoLog << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  return shaderProgram;
}

Renderer::Renderer(int width, int height) : m_width(width), m_height(height) {
  m_shaderProgram = createShaderProgram();
  if (!m_shaderProgram) {
    std::cerr << "❌ Failed to create shader program!\n";
    exit(1);
  }

  float vertices[] = {
      // pos      // tex
      -0.5f, -0.5f, 0.0f, 0.0f, // Bottom-left
      0.5f,  -0.5f, 1.0f, 0.0f, // Bottom-right
      0.5f,  0.5f,  1.0f, 1.0f, // Top-right
      -0.5f, 0.5f,  0.0f, 1.0f  // Top-left
  };
  unsigned int indices[] = {0, 1, 2, 2, 3, 0};

  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);
  glGenBuffers(1, &m_ebo);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  // Position (2 components)
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  // Texture coordinates (2 components)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);

  // Initialize projection (Y-up coordinate system)
  glm::mat4 proj =
      glm::ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);
  glUseProgram(m_shaderProgram);
  glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1,
                     GL_FALSE, glm::value_ptr(proj));

  std::cout << "✅ Renderer initialized (" << width << "x" << height << ")\n";
}

Renderer::~Renderer() {
  if (m_vao)
    glDeleteVertexArrays(1, &m_vao);
  if (m_vbo)
    glDeleteBuffers(1, &m_vbo);
  if (m_ebo)
    glDeleteBuffers(1, &m_ebo);
  if (m_shaderProgram)
    glDeleteProgram(m_shaderProgram);
}

void Renderer::beginFrame() {
  glClearColor(0.05f, 0.02f, 0.15f, 1.0f); // Dark purple background
  glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::endFrame() {}

void Renderer::drawQuad(const Vector2 &pos, const Vector2 &size, float r,
                        float g, float b, float rotation) {
  glUseProgram(m_shaderProgram);
  glUniform3f(glGetUniformLocation(m_shaderProgram, "color"), r, g, b);
  glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), 0);

  // CORRECT ORDER: Translate -> Rotate -> Scale
  glm::mat4 model = glm::mat4(1.0f);
  model =
      glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f)); // Move to position
  model = glm::rotate(model, rotation,
                      glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around Z-axis
  model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f)); // Scale to size

  glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1,
                     GL_FALSE, glm::value_ptr(model));

  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Renderer::drawQuad(const Vector2 &pos, const Vector2 &size,
                        const Texture *texture, float rotation) {
  glUseProgram(m_shaderProgram);

  // 1. Говорим шейдеру использовать текстуру
  glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), 1); // true
  glUniform1i(glGetUniformLocation(m_shaderProgram, "texture0"), 0);   // слот 0

  // 2. Биндим текстуру
  if (texture) {
    texture->bind(0);
  }

  // 3. Матрица модели (Тот же код, что и раньше)
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
  model = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

  glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1,
                     GL_FALSE, glm::value_ptr(model));

  // 4. Рисуем
  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Renderer::setCameraPosition(const Vector2 &pos, const Vector2 &shake) {
  float halfWidth = m_width / 2.0f;
  float halfHeight = m_height / 2.0f;

  // Calculate view bounds with camera position and screen shake
  float left = pos.x - halfWidth + shake.x;
  float right = pos.x + halfWidth + shake.x;
  float bottom = pos.y - halfHeight + shake.y;
  float top = pos.y + halfHeight + shake.y;

  glm::mat4 proj = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
  glUseProgram(m_shaderProgram);
  glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1,
                     GL_FALSE, glm::value_ptr(proj));
}

} // namespace mine
