#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;
uniform vec4 spriteColor;

void main() {
    FragColor = texture(texture1, TexCoord) * spriteColor;
}
