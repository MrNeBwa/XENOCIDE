#include "mine/graphics/Renderer.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace mine {

class ShaderProgram {
public:
    unsigned int id;
    ShaderProgram() {
        const char* vert = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            uniform mat4 projection;
            uniform mat4 model;
            void main() {
                gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
            }
        )";

        const char* frag = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec3 color;
            void main() {
                FragColor = vec4(color, 1.0);
            }
        )";

        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vert, nullptr);
        glCompileShader(vs);

        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &frag, nullptr);
        glCompileShader(fs);

        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);

        glDeleteShader(vs);
        glDeleteShader(fs);
    }
};

static ShaderProgram g_shader;
static unsigned int g_vao = 0;
static unsigned int g_vbo = 0;
static unsigned int g_ebo = 0;

Renderer::Renderer(int width, int height) {
    static bool initialized = false;
    if (!initialized) {
        g_shader = ShaderProgram();

        float vertices[] = {
            -0.5f, -0.5f,
             0.5f, -0.5f,
             0.5f,  0.5f,
            -0.5f,  0.5f
        };
        unsigned int indices[] = {0, 1, 2, 2, 3, 0};

        glGenVertexArrays(1, &g_vao);
        glGenBuffers(1, &g_vbo);
        glGenBuffers(1, &g_ebo);

        glBindVertexArray(g_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        initialized = true;
    }

    glm::mat4 proj = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    glUseProgram(g_shader.id);
    glUniformMatrix4fv(glGetUniformLocation(g_shader.id, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
}

void Renderer::beginFrame() {
    glClearColor(0.05f, 0.02f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::endFrame() {}

void Renderer::drawColoredQuad(const Vector2& pos, const Vector2& size, float r, float g, float b) {
    glUseProgram(g_shader.id);
    glUniform3f(glGetUniformLocation(g_shader.id, "color"), r, g, b);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
    model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(g_shader.id, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(g_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

} // namespace mine
